#include "renderer.h"



HRESULT Renderer::InitDevice(HINSTANCE hInstance, HWND hWnd)
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(hWnd, &rc);
    _width = rc.right - rc.left;
    _height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &_pd3dDevice, &_featureLevel, &_pImmediateContext);

        if (hr == E_INVALIDARG)
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(nullptr, _driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                D3D11_SDK_VERSION, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        }

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = _pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2)
    {
        // DirectX 11.1 or later
        hr = _pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&_pd3dDevice1));
        if (SUCCEEDED(hr))
        {
            (void)_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&_pImmediateContext1));
        }

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.Width = _width;
        sd.Height = _height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        sd.BufferCount = 2;

        hr = dxgiFactory2->CreateSwapChainForHwnd(_pd3dDevice, hWnd, &sd, nullptr, nullptr, &_pSwapChain1);
        if (SUCCEEDED(hr))
        {
            hr = _pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&pSwapChain));
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = _width;
        sd.BufferDesc.Height = _height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(_pd3dDevice, &sd, &pSwapChain);
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    _pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, nullptr);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_width;
    vp.Height = (FLOAT)_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

    if (SUCCEEDED(hr)) 
        hr = _initScene();

    if (SUCCEEDED(hr)) 
    {
        _pCamera = new Camera;
        if (!_pCamera) 
            hr = S_FALSE;
    }

    if (FAILED(hr))
        CleanupDevice();

    return hr;
}

void Renderer::Render()
{
    if (!_updateScene())
        return;

    _pImmediateContext->ClearState();

    ID3D11RenderTargetView* views[] = { _pRenderTargetView };
    _pImmediateContext->OMSetRenderTargets(1, views, nullptr);
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, DirectX::Colors::LightPink);

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = (FLOAT)_width;
    vp.Height = (FLOAT)_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    _pImmediateContext->RSSetViewports(1, &vp);

    D3D11_RECT rect;
    rect.left = 0;
    rect.right = _width;
    rect.top = 0;
    rect.bottom = _height;

    _pImmediateContext->RSSetScissorRects(1, &rect);

    _pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer* vBuffer[] = { _pVertexBuffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };

    _pImmediateContext->IASetVertexBuffers(0, 1, vBuffer, strides, offsets);
    _pImmediateContext->IASetInputLayout(_pInputLayout);
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _pImmediateContext->VSSetConstantBuffers(0, 1, &_pWorld);
    _pImmediateContext->VSSetConstantBuffers(1, 1, &_pView);
    _pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
    _pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
    _pImmediateContext->DrawIndexed(36, 0, 0);


    HRESULT hr = pSwapChain->Present(1, 0);
    assert(SUCCEEDED(hr));
}

void Renderer::CleanupDevice()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();

    if (_pRenderTargetView) _pRenderTargetView->Release();

    if (_pSwapChain1) _pSwapChain1->Release();
    if (pSwapChain) pSwapChain->Release();

    if (_pImmediateContext1) _pImmediateContext1->Release();
    if (_pImmediateContext) _pImmediateContext->Release();

    if (_pd3dDevice1) _pd3dDevice1->Release();
    if (_pd3dDevice) _pd3dDevice->Release();

    if (_pIndexBuffer) _pIndexBuffer->Release();
    if (_pVertexBuffer) _pVertexBuffer->Release();

    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();

    if (_pInputLayout) _pInputLayout->Release();

    if (_pWorld) _pWorld->Release();
    if (_pView) _pView->Release();
    if (_pRasterizerState) _pRasterizerState->Release();

    if (_pCamera) 
    {
        delete _pCamera;
        _pCamera = nullptr;
    }

}

HRESULT Renderer::_setupBackBuffer() 
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    assert(SUCCEEDED(hr));
    if (SUCCEEDED(hr)) 
    {
        hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
        assert(SUCCEEDED(hr));

        SAFE_RELEASE(pBackBuffer);
    }
    return hr;
}

bool Renderer::WinResize(UINT width, UINT height) 
{
    if (_width != width || _height != height) 
    {
        SAFE_RELEASE(_pRenderTargetView);

        HRESULT hr = pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr)) 
        {
            _width = width;
            _height = height;

            hr = _setupBackBuffer();
        }
        return SUCCEEDED(hr);
    }
    return true;
}

HRESULT Renderer::_initScene() 
{
    HRESULT hr = S_OK;

    static const Vertex Vertices[] = {
        {-1.0f,  1.0f, -1.0f, RGB(0, 0, 255)},
        { 1.0f,  1.0f, -1.0f, RGB(0, 255, 0)},
        { 1.0f,  1.0f,  1.0f, RGB(255, 0, 0)},
        {-1.0f,  1.0f,  1.0f, RGB(0, 255, 255)},
        {-1.0f, -1.0f, -1.0f, RGB(255, 0, 255)},
        { 1.0f, -1.0f, -1.0f, RGB(255, 255, 0)},
        { 1.0f, -1.0f,  1.0f, RGB(255, 255, 255)},
        {-1.0f, -1.0f,  1.0f, RGB(0, 0, 0)}
    };

    static const USHORT Indices[] = {
         3,1,0,
        2,1,3,

        0,5,4,
        1,5,0,

        3,4,7,
        0,4,3,

        1,6,5,
        2,6,1,

        2,7,6,
        3,7,2,

        6,4,5,
        7,4,6,
    };

    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0,  DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0} };


    if (SUCCEEDED(hr)) 
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Vertices;
        data.SysMemPitch = sizeof(Vertices);
        data.SysMemSlicePitch = 0;

        hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pVertexBuffer);
        assert(SUCCEEDED(hr));
    }

    if (SUCCEEDED(hr)) 
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Indices;
        data.SysMemPitch = sizeof(Indices);
        data.SysMemSlicePitch = 0;

        hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pIndexBuffer);
        assert(SUCCEEDED(hr));
    }

    ID3D10Blob* vShaderBuffer = nullptr;
    ID3D10Blob* pShaderBuffer = nullptr;

    int flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    if (SUCCEEDED(hr)) 
    {
        hr = D3DCompileFromFile(L"VS.hlsl", nullptr, nullptr, "vs", "vs_5_0", flags, 0, &vShaderBuffer, NULL);
        hr = _pd3dDevice->CreateVertexShader(vShaderBuffer->GetBufferPointer(), vShaderBuffer->GetBufferSize(), NULL, &_pVertexShader);
    }
    if (SUCCEEDED(hr)) 
    {
        hr = D3DCompileFromFile(L"PS.hlsl", nullptr, nullptr, "ps", "ps_5_0", flags, 0, &pShaderBuffer, NULL);
        hr = _pd3dDevice->CreatePixelShader(pShaderBuffer->GetBufferPointer(), pShaderBuffer->GetBufferSize(), NULL, &_pPixelShader);
    }
    if (SUCCEEDED(hr)) 
    {
        hr = _pd3dDevice->CreateInputLayout(InputDesc, 2, vShaderBuffer->GetBufferPointer(), vShaderBuffer->GetBufferSize(), &_pInputLayout);
    }

    SAFE_RELEASE(vShaderBuffer);
    SAFE_RELEASE(pShaderBuffer);

    if (SUCCEEDED(hr)) 
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(WorldMatrixBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        WorldMatrixBuffer worldMatrixBuffer;
        worldMatrixBuffer.worldMatrix = DirectX::XMMatrixIdentity();

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &worldMatrixBuffer;
        data.SysMemPitch = sizeof(worldMatrixBuffer);
        data.SysMemSlicePitch = 0;

        hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pWorld);
    }
    if (SUCCEEDED(hr)) 
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(ViewMatrixBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        hr = _pd3dDevice->CreateBuffer(&desc, nullptr, &_pView);
    }
    if (SUCCEEDED(hr)) 
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.AntialiasedLineEnable = false;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.0f;
        desc.FrontCounterClockwise = false;
        desc.DepthClipEnable = true;
        desc.ScissorEnable = false;
        desc.MultisampleEnable = false;
        desc.SlopeScaledDepthBias = 0.0f;

        hr = _pd3dDevice->CreateRasterizerState(&desc, &_pRasterizerState);
    }

    return hr;
}


bool Renderer::_updateScene() 
{
    HRESULT result;

    static float t = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0) 
        timeStart = timeCur;
    t = (timeCur - timeStart) / 1000.0f;

    WorldMatrixBuffer worldMatrixBuffer;
    worldMatrixBuffer.worldMatrix = XMMatrixRotationY(t);

    _pImmediateContext->UpdateSubresource(_pWorld, 0, nullptr, &worldMatrixBuffer, 0, 0);

    XMMATRIX mView = _pCamera->GetViewMatrix();

    XMMATRIX mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV2, _width / (FLOAT)_height, 0.01f, 100.0f);

    D3D11_MAPPED_SUBRESOURCE subresource;
    result = _pImmediateContext->Map(_pView, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    if (SUCCEEDED(result)) 
    {
        ViewMatrixBuffer& sceneBuffer = *reinterpret_cast<ViewMatrixBuffer*>(subresource.pData);
        sceneBuffer.viewProjectionMatrix = XMMatrixMultiply(mView, mProjection);
        _pImmediateContext->Unmap(_pView, 0);
    }

    return SUCCEEDED(result);
}

void Renderer::MouseButtonDown(WPARAM wParam, LPARAM lParam) 
{
    _mouseButtonPressed = true;
    _prevMousePos.x = GET_X_LPARAM(lParam);
    _prevMousePos.y = GET_Y_LPARAM(lParam);
}

void Renderer::MouseButtonUp(WPARAM wParam, LPARAM lParam) 
{
    _mouseButtonPressed = false;
    _prevMousePos.x = GET_X_LPARAM(lParam);
    _prevMousePos.y = GET_Y_LPARAM(lParam);
}

void Renderer::MouseMoved(WPARAM wParam, LPARAM lParam)
{
    if (_mouseButtonPressed) {
        _pCamera->ChangePos((GET_X_LPARAM(lParam) - _prevMousePos.x) / 100.0f, (GET_Y_LPARAM(lParam) - _prevMousePos.y) / 100.0f);
        _prevMousePos.x = GET_X_LPARAM(lParam);
        _prevMousePos.y = GET_Y_LPARAM(lParam);
    }
        
}