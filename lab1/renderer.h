#pragma once

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>
#include "camera.h"
#include <DirectXMath.h>
#include <windowsx.h>
#include <vector>
#include <algorithm>
#include "DDSTextureLoader11.h"

using namespace DirectX;

#define SAFE_RELEASE(p) \
if (p != NULL) { \
    p->Release(); \
    p = NULL;\
}

struct TexVertex
{
	XMFLOAT3 pos;
	XMFLOAT2 uv;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
};

struct WorldMatrixBuffer 
{
	XMMATRIX worldMatrix;
	XMFLOAT4 shine;
};

struct ColoredObjMatrixBuffer
{
	XMMATRIX worldMatrix;
	XMFLOAT4 color;
};

struct Light 
{
	XMFLOAT4 pos;
	XMFLOAT4 color;
};

struct ViewMatrixBuffer 
{
	XMMATRIX viewProjectionMatrix;
	XMFLOAT4 cameraPos;
	XMINT4 lightParams;
	Light lights[4];
	XMFLOAT4 ambientColor;
};


struct SkyboxWorldMatrixBuffer 
{
	XMMATRIX worldMatrix;
	XMFLOAT4 size;
};


struct SkyboxViewMatrixBuffer 
{
	XMMATRIX viewProjectionMatrix;
	XMFLOAT4 cameraPos;
};


struct SphereVertex
{
	float x, y, z;
};


static const XMFLOAT4 TransVertices[] = {
	 {0, -2.5, -2.5, 1.0},
	 {0,  2.5, 0, 1.0},
	 {0,  -2.5,  2.5, 1.0}
};

class Renderer 
{
public:
	void Render();
	void CleanupDevice();
	bool WinResize(UINT width, UINT height);
	HRESULT InitDevice(HINSTANCE hInstance, HWND hWnd);
	void MouseButtonDown(WPARAM wParam, LPARAM lParam);
	void MouseButtonUp(WPARAM wParam, LPARAM lParam);
	void MouseMoved(WPARAM wParam, LPARAM lParam);

private:
	D3D_DRIVER_TYPE         _driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL       _featureLevel = D3D_FEATURE_LEVEL_11_0;

	ID3D11Device* _pd3dDevice = nullptr;
	ID3D11Device1* _pd3dDevice1 = nullptr;

	ID3D11DeviceContext* _pImmediateContext = nullptr;
	ID3D11DeviceContext1* _pImmediateContext1 = nullptr;
	
	IDXGISwapChain* _pSwapChain = nullptr;
	IDXGISwapChain1* _pSwapChain1 = nullptr;

	ID3D11RenderTargetView* _pRenderTargetView = nullptr;

	ID3D11Buffer* _pIndexBuffer = nullptr;
	ID3D11Buffer* _pVertexBuffer = nullptr;
	ID3D11VertexShader* _pVertexShader = nullptr;
	ID3D11PixelShader* _pPixelShader = nullptr;
	ID3D11InputLayout* _pInputLayout = nullptr;
	ID3D11ShaderResourceView* _pTexture = nullptr;
	ID3D11ShaderResourceView* _pNormTexture = nullptr;
	ID3D11Buffer* _pWorldMatrixBuffer[2] = { nullptr, nullptr };
	ID3D11Buffer* _pViewMatrixBuffer = nullptr;

	ID3D11Buffer* _pSkyboxIndexBuffer = nullptr;
	ID3D11Buffer* _pSkyboxVertexBuffer = nullptr;
	ID3D11VertexShader* _pSkyboxVertexShader = nullptr;
	ID3D11PixelShader* _pSkyboxPixelShader = nullptr;
	ID3D11InputLayout* _pSkyboxInputLayout = nullptr;
	ID3D11ShaderResourceView* _pSkyboxTexture = nullptr;
	ID3D11Buffer* _pSkyboxWorldMatrixBuffer = nullptr;
	ID3D11Buffer* _pSkyboxViewMatrixBuffer = nullptr;

	UINT _width;
	UINT _height;

	ID3D11Buffer* _pTIndexBuffer = nullptr;
	ID3D11Buffer* _pTVertexBuffer = nullptr;
	ID3D11VertexShader* _pTVertexShader = nullptr;
	ID3D11PixelShader* _pTPixelShader = nullptr;
	ID3D11InputLayout* _pTInputLayout = nullptr;
	ID3D11Buffer* _pTWorldMatrixBuffer[2] = { nullptr, nullptr };


	ID3D11Buffer* _pLightIndexBuffer = nullptr;
	ID3D11Buffer* _pLightVertexBuffer = nullptr;
	ID3D11VertexShader* _pLightVertexShader = nullptr;
	ID3D11PixelShader* _pLightPixelShader = nullptr;
	ID3D11InputLayout* _pLightInputLayout = nullptr;
	ID3D11Buffer* _pLightWorldMatrixBuffer[4] = { nullptr, nullptr, nullptr, nullptr };
	ID3D11Buffer* _pLightViewMatrixBuffer = nullptr;
	
	ID3D11RasterizerState* _pRasterizerState = nullptr;

	ID3D11Texture2D* _pDepthBuffer = nullptr;
	ID3D11DepthStencilView* _pDepthBufferDSV = nullptr;
	ID3D11DepthStencilState* _pDepthState = nullptr;
	ID3D11DepthStencilState* _pZeroDepthState = nullptr;
	  
	ID3D11BlendState* _pBlendState = nullptr;

	Camera* _pCamera = nullptr;
	ID3D11SamplerState* _pSampler = nullptr;
	ColoredObjMatrixBuffer _TWorld[2];

	bool _mouseButtonPressed = false;
	POINT _prevMousePos;

	UINT _numSphereTriangles = 0.0;
	float _radius = 0.2;

	std::vector<Light> _pLight;

	HRESULT _setupBackBuffer();
	HRESULT _setupDepthBuffer();
	HRESULT _initScene();
	float _getDistToTrans(XMMATRIX worldMatrix, XMFLOAT3 cameraPos);
	bool _updateScene();
};

class D3DInclude : public ID3DInclude
{
	STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName,
		LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
	{
		FILE* pFile = nullptr;
		fopen_s(&pFile, pFileName, "rb");
		assert(pFile != nullptr);
		if (pFile == nullptr)
		{
			return E_FAIL;
		}

		_fseeki64(pFile, 0, SEEK_END);
		long long size = _ftelli64(pFile);
		_fseeki64(pFile, 0, SEEK_SET);

		char* pData = new char[size + 1];

		size_t rd = fread(pData, 1, size, pFile);
		assert(rd == (size_t)size);
		fclose(pFile);

		*ppData = pData;
		*pBytes = (UINT)size;
		return S_OK;
	}
	STDMETHOD(Close)(THIS_ LPCVOID pData)
	{
		free(const_cast<void*>(pData));
		return S_OK;
	}
};
