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
            hr = _pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&_pSwapChain));
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

        hr = dxgiFactory->CreateSwapChain(_pd3dDevice, &sd, &_pSwapChain);
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    if (SUCCEEDED(hr))
        hr = _setupBackBuffer();

    if (SUCCEEDED(hr))
        hr = _setupDepthBuffer();

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
    _pImmediateContext->OMSetRenderTargets(1, views, _pDepthBufferDSV);
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, Colors::LightPink);
    _pImmediateContext->ClearDepthStencilView(_pDepthBufferDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);

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
    _pImmediateContext->RSSetState(_pRasterizerState);
    ID3D11SamplerState* samplers[] = { _pSampler };
    _pImmediateContext->PSSetSamplers(0, 1, samplers);
    {  
        _pImmediateContext->OMSetDepthStencilState(_pSkyboxDepthState, 0);
        ID3D11ShaderResourceView* resources[] = {_pSkyboxTexture };
        _pImmediateContext->PSSetShaderResources(0, 1, resources);

        _pImmediateContext->IASetIndexBuffer(_pSkyboxIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        ID3D11Buffer* vBuffers[] = { _pSkyboxVertexBuffer };
        UINT strides[] = { 12 };
        UINT offsets[] = { 0 };
        _pImmediateContext->IASetVertexBuffers(0, 1, vBuffers, strides, offsets);
        _pImmediateContext->IASetInputLayout(_pSkyboxInputLayout);
        _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _pImmediateContext->VSSetShader(_pSkyboxVertexShader, nullptr, 0);
        _pImmediateContext->VSSetConstantBuffers(0, 1, &_pSkyboxWorldMatrix);
        _pImmediateContext->VSSetConstantBuffers(1, 1, &_pSkyboxViewMatrix);
        _pImmediateContext->PSSetShader(_pSkyboxPixelShader, nullptr, 0);

        _pImmediateContext->DrawIndexed(_numSphereTriangles * 3, 0, 0);
    }
    {
        ID3D11ShaderResourceView* resources[] = { _pTexture };
        _pImmediateContext->PSSetShaderResources(0, 1, resources);
        _pImmediateContext->OMSetDepthStencilState(_pDepthState, 0);

        _pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        ID3D11Buffer* vBuffers[] = { _pVertexBuffer };
        UINT strides[] = { 20 };
        UINT offsets[] = { 0 };
        _pImmediateContext->IASetVertexBuffers(0, 1, vBuffers, strides, offsets);
        _pImmediateContext->IASetInputLayout(_pInputLayout);
        _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        _pImmediateContext->VSSetConstantBuffers(0, 1, &_pWorldMatrix[0]);
        _pImmediateContext->VSSetConstantBuffers(1, 1, &_pViewMatrix);
        _pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
        _pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
        _pImmediateContext->DrawIndexed(36, 0, 0);
        _pImmediateContext->VSSetConstantBuffers(0, 1, &_pWorldMatrix[1]);
        _pImmediateContext->DrawIndexed(36, 0, 0);
    }
    {
        _pImmediateContext->IASetIndexBuffer(_pTIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
        ID3D11Buffer* vertexBuffers[] = { _pTVertexBuffer };
        UINT strides[] = { 16 };
        UINT offsets[] = { 0 };
        _pImmediateContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
        _pImmediateContext->IASetInputLayout(_pTInputLayout);

        _pImmediateContext->VSSetShader(_pTVertexShader, nullptr, 0);
        _pImmediateContext->PSSetShader(_pTPixelShader, nullptr, 0);
        _pImmediateContext->VSSetConstantBuffers(1, 1, &_pViewMatrix);

        _pImmediateContext->OMSetBlendState(_pBlendState, nullptr, 0xFFFFFFFF);
        _pImmediateContext->VSSetConstantBuffers(0, 1, &_pTWorldMatrix[0]);
        _pImmediateContext->DrawIndexed(3, 0, 0);

        _pImmediateContext->VSSetConstantBuffers(0, 1, &_pTWorldMatrix[1]);
        _pImmediateContext->DrawIndexed(3, 0, 0);

    }
    

    HRESULT hr = _pSwapChain->Present(0, 0);
    assert(SUCCEEDED(hr));
}

void Renderer::CleanupDevice()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();

    if (_pRenderTargetView) _pRenderTargetView->Release();

    if (_pSwapChain1) _pSwapChain1->Release();
    if (_pSwapChain) _pSwapChain->Release();

    if (_pImmediateContext1) _pImmediateContext1->Release();
    if (_pImmediateContext) _pImmediateContext->Release();

    if (_pd3dDevice1) _pd3dDevice1->Release();
    if (_pd3dDevice) _pd3dDevice->Release();

    if (_pIndexBuffer) _pIndexBuffer->Release();
    if (_pVertexBuffer) _pVertexBuffer->Release();
    if (_pSkyboxIndexBuffer) _pSkyboxIndexBuffer->Release();
    if (_pSkyboxVertexBuffer) _pSkyboxVertexBuffer->Release();

    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();
    if (_pSkyboxVertexShader) _pSkyboxVertexShader->Release();
    if (_pSkyboxPixelShader) _pSkyboxPixelShader->Release();

    if (_pInputLayout) _pInputLayout->Release();
    if (_pSkyboxInputLayout) _pSkyboxInputLayout->Release();

    if (_pWorldMatrix[0]) _pWorldMatrix[0]->Release();
    if (_pWorldMatrix[1]) _pWorldMatrix[1]->Release();
    if (_pViewMatrix) _pViewMatrix->Release();
    if (_pSkyboxWorldMatrix) _pSkyboxWorldMatrix->Release();
    if (_pSkyboxViewMatrix) _pSkyboxViewMatrix->Release();
    if (_pRasterizerState) _pRasterizerState->Release();

    if (_pTexture) _pTexture->Release();
    if (_pSkyboxTexture) _pSkyboxTexture->Release();

    if (_pDepthBuffer) _pDepthBuffer->Release();
    if (_pDepthBufferDSV) _pDepthBufferDSV->Release();
    if (_pDepthState) _pDepthState->Release();
    if (_pSkyboxDepthState) _pSkyboxDepthState->Release();
    if (_pBlendState) _pBlendState->Release();

    if (_pTIndexBuffer) _pTIndexBuffer->Release();
    if (_pTVertexBuffer) _pTVertexBuffer->Release();
    if (_pTVertexShader) _pTVertexShader->Release();
    if (_pTPixelShader) _pTPixelShader->Release();
    if (_pTInputLayout) _pTInputLayout->Release();
    if (_pTWorldMatrix[0]) _pTWorldMatrix[0]->Release();
    if (_pTWorldMatrix[1]) _pTWorldMatrix[1]->Release();

    if (_pCamera) 
    {
        delete _pCamera;
        _pCamera = nullptr;
    }

}

HRESULT Renderer::_setupBackBuffer() 
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (SUCCEEDED(hr)) 
    {
        hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
        assert(SUCCEEDED(hr));

        SAFE_RELEASE(pBackBuffer);
    }
    return hr;
}

HRESULT Renderer::_setupDepthBuffer() 
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.ArraySize = 1;
    desc.MipLevels = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Height = _height;
    desc.Width = _width;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = _pd3dDevice->CreateTexture2D(&desc, NULL, &_pDepthBuffer);

    if (SUCCEEDED(hr)) 
        hr = _pd3dDevice->CreateDepthStencilView(_pDepthBuffer, NULL, &_pDepthBufferDSV);

    return hr;
}

bool Renderer::WinResize(UINT width, UINT height) 
{
    if (!_pSwapChain)
        return false;
    if (_width != width || _height != height) 
    {
        SAFE_RELEASE(_pRenderTargetView);
        SAFE_RELEASE(_pDepthBufferDSV);
        SAFE_RELEASE(_pDepthBuffer);

        HRESULT hr = _pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr)) 
        {
            _width = width;
            _height = height;

            hr = _setupBackBuffer();

            if (SUCCEEDED(hr))
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
        {-1.0, -1.0,  1.0, 0, 1},
        { 1.0, -1.0,  1.0, 1, 1},
        { 1.0, -1.0, -1.0, 1, 0},
        {-1.0, -1.0, -1.0, 0, 0},

        {-1.0,  1.0, -1.0, 0, 1},
        { 1.0,  1.0, -1.0, 1, 1},
        { 1.0,  1.0,  1.0, 1, 0},
        {-1.0,  1.0,  1.0, 0, 0},

        { 1.0, -1.0, -1.0, 0, 1},
        { 1.0, -1.0,  1.0, 1, 1},
        { 1.0,  1.0,  1.0, 1, 0},
        { 1.0,  1.0, -1.0, 0, 0},

        {-1.0, -1.0,  1.0, 0, 1},
        {-1.0, -1.0, -1.0, 1, 1},
        {-1.0,  1.0, -1.0, 1, 0},
        {-1.0,  1.0,  1.0, 0, 0},

        { 1.0, -1.0,  1.0, 0, 1},
        {-1.0, -1.0,  1.0, 1, 1},
        {-1.0,  1.0,  1.0, 1, 0},
        { 1.0,  1.0,  1.0, 0, 0},

        {-1.0, -1.0, -1.0, 0, 1},
        { 1.0, -1.0, -1.0, 1, 1},
        { 1.0,  1.0, -1.0, 1, 0},
        {-1.0,  1.0, -1.0, 0, 0}
    };
    static const USHORT Indices[] = {
        0, 2, 1, 0, 3, 2,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22
    };
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0} };

    static const TransparentVertex VerticesT[] = {
       {0, -2.5, -2.5, RGB(0, 0, 255)},
       {0,  2.5, 0, RGB(0, 255, 0)},
       {0,  -2.5,  2.5, RGB(255, 0, 0)}
    };
    static const USHORT IndicesT[] = {
        0, 2, 1
    };
    static const D3D11_INPUT_ELEMENT_DESC InputDescT[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0,  DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    UINT LatLines = 20, LongLines = 20;
    UINT numSphereVertices = ((LatLines - 2) * LongLines) + 2;
    _numSphereTriangles = ((LatLines - 3) * (LongLines) * 2) + (LongLines * 2);

    float phi = 0.0f;
    float theta = 0.0f;

    std::vector<SkyboxVertex> vertices(numSphereVertices);

    XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    vertices[0].x = 0.0f;
    vertices[0].y = 0.0f;
    vertices[0].z = 1.0f;

    for (UINT i = 0; i < LatLines - 2; i++)
    {
        theta = (i + 1) * (XM_PI / (LatLines - 1));
        XMMATRIX Rotationx = XMMatrixRotationX(theta);
        for (UINT j = 0; j < LongLines; j++) 
        {
            phi = j * (XM_2PI / LongLines);
            XMMATRIX Rotationy = XMMatrixRotationZ(phi);
            currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (Rotationx * Rotationy));
            currVertPos = XMVector3Normalize(currVertPos);
            vertices[i * (__int64)LongLines + j + 1].x = XMVectorGetX(currVertPos);
            vertices[i * (__int64)LongLines + j + 1].y = XMVectorGetY(currVertPos);
            vertices[i * (__int64)LongLines + j + 1].z = XMVectorGetZ(currVertPos);
        }
    }

    vertices[(__int64)numSphereVertices - 1].x = 0.0f;
    vertices[(__int64)numSphereVertices - 1].y = 0.0f;
    vertices[(__int64)numSphereVertices - 1].z = -1.0f;

    std::vector<UINT> indices((__int64)_numSphereTriangles * 3);

    UINT k = 0;
    for (UINT i = 0; i < LongLines - 1; i++) 
    {
        indices[k] = 0;
        indices[(__int64)k + 2] = i + 1;
        indices[(__int64)k + 1] = i + 2;
        k += 3;
    }
    indices[k] = 0;
    indices[(__int64)k + 2] = LongLines;
    indices[(__int64)k + 1] = 1;
    k += 3;

    for (UINT i = 0; i < LatLines - 3; i++) 
    {
        for (UINT j = 0; j < LongLines - 1; j++) 
        {
            indices[k] = i * LongLines + j + 1;
            indices[(__int64)k + 1] = i * LongLines + j + 2;
            indices[(__int64)k + 2] = (i + 1) * LongLines + j + 1;

            indices[(__int64)k + 3] = (i + 1) * LongLines + j + 1;
            indices[(__int64)k + 4] = i * LongLines + j + 2;
            indices[(__int64)k + 5] = (i + 1) * LongLines + j + 2;

            k += 6;
        }

        indices[k] = (i * LongLines) + LongLines;
        indices[(__int64)k + 1] = (i * LongLines) + 1;
        indices[(__int64)k + 2] = ((i + 1) * LongLines) + LongLines;

        indices[(__int64)k + 3] = ((i + 1) * LongLines) + LongLines;
        indices[(__int64)k + 4] = (i * LongLines) + 1;
        indices[(__int64)k + 5] = ((i + 1) * LongLines) + 1;

        k += 6;
    }

    for (UINT i = 0; i < LongLines - 1; i++) 
    {
        indices[k] = numSphereVertices - 1;
        indices[(__int64)k + 2] = (numSphereVertices - 1) - (i + 1);
        indices[(__int64)k + 1] = (numSphereVertices - 1) - (i + 2);
        k += 3;
    }

    indices[k] = numSphereVertices - 1;
    indices[(__int64)k + 2] = (numSphereVertices - 1) - LongLines;
    indices[(__int64)k + 1] = numSphereVertices - 2;

    static const D3D11_INPUT_ELEMENT_DESC SkyboxInputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

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
    }

    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;
    int flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    if (SUCCEEDED(hr))
    {
        hr = D3DCompileFromFile(L"VS.hlsl", nullptr, nullptr, "vs", "vs_5_0", flags, 0, &vertexShaderBuffer, nullptr);
        if (SUCCEEDED(hr)) 
        {
            hr = _pd3dDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &_pVertexShader);
        }
    }
    if (SUCCEEDED(hr)) 
    {
        hr = D3DCompileFromFile(L"PS.hlsl", nullptr, nullptr, "ps", "ps_5_0", flags, 0, &pixelShaderBuffer, nullptr);
        if (SUCCEEDED(hr)) 
        {
            hr = _pd3dDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, &_pPixelShader);
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = _pd3dDevice->CreateInputLayout(InputDesc, 2, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &_pInputLayout);
    }

    SAFE_RELEASE(vertexShaderBuffer);
    SAFE_RELEASE(pixelShaderBuffer);

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
        worldMatrixBuffer.worldMatrix = XMMatrixIdentity();

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &worldMatrixBuffer;
        data.SysMemPitch = sizeof(worldMatrixBuffer);
        data.SysMemSlicePitch = 0;

        hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pWorldMatrix[0]);
        if (SUCCEEDED(hr)) 
            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pWorldMatrix[1]);
        if (SUCCEEDED(hr)) 
            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pTWorldMatrix[0]);
        if (SUCCEEDED(hr)) 
            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pTWorldMatrix[1]);
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

        hr = _pd3dDevice->CreateBuffer(&desc, nullptr, &_pViewMatrix);
    }
    {
        if (SUCCEEDED(hr))
        {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(SkyboxVertex) * numSphereVertices;
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA data;
            ZeroMemory(&data, sizeof(data));
            data.pSysMem = &vertices[0];
            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pSkyboxVertexBuffer);
        }

        if (SUCCEEDED(hr))
        {
            D3D11_BUFFER_DESC desc = {};
            ZeroMemory(&desc, sizeof(desc));
            desc.ByteWidth = sizeof(UINT) * _numSphereTriangles * 3;
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = &indices[0];

            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pSkyboxIndexBuffer);
        }

        ID3D10Blob* vertexShaderBuffer = nullptr;
        ID3D10Blob* pixelShaderBuffer = nullptr;
        int flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        if (SUCCEEDED(hr))
        {
            hr = D3DCompileFromFile(L"CubeMap_VS.hlsl", nullptr, nullptr, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, nullptr);
            if (SUCCEEDED(hr))
            {
                hr = _pd3dDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &_pSkyboxVertexShader);
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = D3DCompileFromFile(L"CubeMap_PS.hlsl", nullptr, nullptr, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, nullptr);
            if (SUCCEEDED(hr))
            {
                hr = _pd3dDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &_pSkyboxPixelShader);
            }
        }
        if (SUCCEEDED(hr))
        {
            int numElements = sizeof(SkyboxInputDesc) / sizeof(SkyboxInputDesc[0]);
            hr = _pd3dDevice->CreateInputLayout(SkyboxInputDesc, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &_pSkyboxInputLayout);
        }

        SAFE_RELEASE(vertexShaderBuffer);
        SAFE_RELEASE(pixelShaderBuffer);

        if (SUCCEEDED(hr))
        {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(SkyboxWorldMatrixBuffer);
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            SkyboxWorldMatrixBuffer skyboxWorldMatrixBuffer;
            skyboxWorldMatrixBuffer.worldMatrix = XMMatrixIdentity();
            skyboxWorldMatrixBuffer.size = XMFLOAT4(_radius, 0.0f, 0.0f, 0.0f);

            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = &skyboxWorldMatrixBuffer;
            data.SysMemPitch = sizeof(skyboxWorldMatrixBuffer);
            data.SysMemSlicePitch = 0;

            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pSkyboxWorldMatrix);
        }
        if (SUCCEEDED(hr))
        {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(SkyboxViewMatrixBuffer);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            hr = _pd3dDevice->CreateBuffer(&desc, nullptr, &_pSkyboxViewMatrix);
        }
    }
    {
        if (SUCCEEDED(hr)) 
        {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(VerticesT);
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = &VerticesT;
            data.SysMemPitch = sizeof(VerticesT);
            data.SysMemSlicePitch = 0;

            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pTVertexBuffer);
        }
        if (SUCCEEDED(hr)) 
        {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(IndicesT);
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = &IndicesT;
            data.SysMemPitch = sizeof(IndicesT);
            data.SysMemSlicePitch = 0;

            hr = _pd3dDevice->CreateBuffer(&desc, &data, &_pTIndexBuffer);
        }

        ID3D10Blob* vertexShaderBuffer = nullptr;
        ID3D10Blob* pixelShaderBuffer = nullptr;
        int flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        if (SUCCEEDED(hr)) 
        {
            hr = D3DCompileFromFile(L"Transparent_VS.hlsl", NULL, NULL, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
            if (SUCCEEDED(hr))
            {
                hr = _pd3dDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &_pTVertexShader);
            }
        }
        if (SUCCEEDED(hr)) 
        {
            hr = D3DCompileFromFile(L"Transparent_PS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
            if (SUCCEEDED(hr))
            {
                hr = _pd3dDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &_pTPixelShader);
            }
        }
        if (SUCCEEDED(hr)) 
        {
            int numElements = sizeof(InputDescT) / sizeof(InputDescT[0]);
            hr = _pd3dDevice->CreateInputLayout(InputDescT, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &_pTInputLayout);
        }

        SAFE_RELEASE(vertexShaderBuffer);
        SAFE_RELEASE(pixelShaderBuffer);
    }
    if (SUCCEEDED(hr)) 
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.AntialiasedLineEnable = false;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.0f;
        desc.FrontCounterClockwise = false;
        desc.DepthClipEnable = true;
        desc.ScissorEnable = false;
        desc.MultisampleEnable = false;
        desc.SlopeScaledDepthBias = 0.0f;

        hr = _pd3dDevice->CreateRasterizerState(&desc, &_pRasterizerState);
    }
    if (SUCCEEDED(hr)) 
    {
        hr = CreateDDSTextureFromFile(_pd3dDevice, _pImmediateContext, L"./cot.dds", nullptr, &_pTexture);
    }
    if (SUCCEEDED(hr)) 
    {
        hr = CreateDDSTextureFromFileEx(_pd3dDevice, _pImmediateContext, L"./skybox.dds",
            0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE,
            DDS_LOADER_DEFAULT, nullptr, &_pSkyboxTexture);
    }
    if (SUCCEEDED(hr)) 
    {
        D3D11_SAMPLER_DESC desc = {};

        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MinLOD = -D3D11_FLOAT32_MAX;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;

        hr = _pd3dDevice->CreateSamplerState(&desc, &_pSampler);
    }
    if (SUCCEEDED(hr))
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = { };
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_GREATER;
        dsDesc.StencilEnable = FALSE;

        hr = _pd3dDevice->CreateDepthStencilState(&dsDesc, &_pDepthState);
    }
    if (SUCCEEDED(hr)) 
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = { };
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        dsDesc.StencilEnable = FALSE;

        hr = _pd3dDevice->CreateDepthStencilState(&dsDesc, &_pSkyboxDepthState);
    }
    if (SUCCEEDED(hr)) 
    {
        D3D11_BLEND_DESC desc = { 0 };
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED |
            D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;

        hr = _pd3dDevice->CreateBlendState(&desc, &_pBlendState);
    }

    return hr;
}

bool Renderer::_updateScene() 
{
    HRESULT hr;

    static float t = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0) 
        timeStart = timeCur;
    t = (timeCur - timeStart) / 1000.0f;

    WorldMatrixBuffer worldMatrixBuffer;
    worldMatrixBuffer.worldMatrix = XMMatrixRotationY(t);

    _pImmediateContext->UpdateSubresource(_pWorldMatrix[0], 0, nullptr, &worldMatrixBuffer, 0, 0);

    worldMatrixBuffer.worldMatrix = XMMatrixTranslation(4.0f, 0.0f, 0.0f);
    _pImmediateContext->UpdateSubresource(_pWorldMatrix[1], 0, nullptr, &worldMatrixBuffer, 0, 0);

    worldMatrixBuffer.worldMatrix = XMMatrixTranslation(2.0f, sin(t), 0.0f);
    _pImmediateContext->UpdateSubresource(_pTWorldMatrix[0], 0, nullptr, &worldMatrixBuffer, 0, 0);

    worldMatrixBuffer.worldMatrix = XMMatrixTranslation(-2.0f, 0.0f, sin(t));
    _pImmediateContext->UpdateSubresource(_pTWorldMatrix[1], 0, nullptr, &worldMatrixBuffer, 0, 0);
   
    XMMATRIX mView = _pCamera->GetViewMatrix();

    XMMATRIX mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV2, _width / (FLOAT)_height, 100.0f, 0.01f);

    D3D11_MAPPED_SUBRESOURCE subresource, skyboxSubresource;
    hr = _pImmediateContext->Map(_pViewMatrix , 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    if (SUCCEEDED(hr)) 
    {
        ViewMatrixBuffer& sceneBuffer = *reinterpret_cast<ViewMatrixBuffer*>(subresource.pData);
        sceneBuffer.viewProjectionMatrix = XMMatrixMultiply(mView, mProjection);
        _pImmediateContext->Unmap(_pViewMatrix, 0);
    }
    if (SUCCEEDED(hr)) 
    {
        SkyboxWorldMatrixBuffer skyboxWorldMatrixBuffer;

        skyboxWorldMatrixBuffer.worldMatrix = XMMatrixIdentity();
        skyboxWorldMatrixBuffer.size = XMFLOAT4(_radius, 0.0f, 0.0f, 0.0f);

        _pImmediateContext->UpdateSubresource(_pSkyboxWorldMatrix, 0, nullptr, &skyboxWorldMatrixBuffer, 0, 0);

        hr = _pImmediateContext->Map(_pSkyboxViewMatrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &skyboxSubresource);
    }
    if (SUCCEEDED(hr)) 
    {
        SkyboxViewMatrixBuffer& skyboxSceneBuffer = *reinterpret_cast<SkyboxViewMatrixBuffer*>(skyboxSubresource.pData);
        skyboxSceneBuffer.viewProjectionMatrix = XMMatrixMultiply(mView, mProjection);
        XMFLOAT3 cameraPos = _pCamera->GetPos();
        skyboxSceneBuffer.cameraPos = XMFLOAT4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
        _pImmediateContext->Unmap(_pSkyboxViewMatrix, 0);
    }

    return SUCCEEDED(hr);
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
    if (_mouseButtonPressed) 
    {
        _pCamera->ChangePos((GET_X_LPARAM(lParam) - _prevMousePos.x) / 100.0f, (GET_Y_LPARAM(lParam) - _prevMousePos.y) / 100.0f);
        _prevMousePos.x = GET_X_LPARAM(lParam);
        _prevMousePos.y = GET_Y_LPARAM(lParam);
    }
        
}