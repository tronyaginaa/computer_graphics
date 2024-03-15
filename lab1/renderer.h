#pragma once

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>
#include "camera.h"
#include <DirectXMath.h>
#include <windowsx.h>
#include <vector>
#include "DDSTextureLoader11.h"

using namespace DirectX;

#define SAFE_RELEASE(p) \
if (p != NULL) { \
    p->Release(); \
    p = NULL;\
}

struct WorldMatrixBuffer 
{
	XMMATRIX worldMatrix;
};

struct ViewMatrixBuffer 
{
	XMMATRIX viewProjectionMatrix;
};

struct Vertex 
{
	float x, y, z;
	float u, v;
};

struct SkyboxWorldMatrixBuffer {
	XMMATRIX worldMatrix;
	XMFLOAT4 size;
};

struct SkyboxViewMatrixBuffer {
	XMMATRIX viewProjectionMatrix;
	XMFLOAT4 cameraPos;
};

struct SkyboxVertex {
	float x, y, z;
};

struct TransparentVertex {
	float x, y, z;
	COLORREF color;
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
	ID3D11Buffer* _pWorldMatrix[2] = { nullptr, nullptr };
	ID3D11Buffer* _pViewMatrix = nullptr;

	ID3D11Buffer* _pSkyboxIndexBuffer = nullptr;
	ID3D11Buffer* _pSkyboxVertexBuffer = nullptr;
	ID3D11VertexShader* _pSkyboxVertexShader = nullptr;
	ID3D11PixelShader* _pSkyboxPixelShader = nullptr;
	ID3D11InputLayout* _pSkyboxInputLayout = nullptr;
	ID3D11ShaderResourceView* _pSkyboxTexture = nullptr;
	ID3D11Buffer* _pSkyboxWorldMatrix = nullptr;
	ID3D11Buffer* _pSkyboxViewMatrix = nullptr;

	UINT _width;
	UINT _height;

	ID3D11Buffer* _pTIndexBuffer = nullptr;
	ID3D11Buffer* _pTVertexBuffer = nullptr;
	ID3D11VertexShader* _pTVertexShader = nullptr;
	ID3D11PixelShader* _pTPixelShader = nullptr;
	ID3D11InputLayout* _pTInputLayout = nullptr;
	ID3D11Buffer* _pTWorldMatrix[2] = { nullptr, nullptr };
	
	ID3D11RasterizerState* _pRasterizerState = nullptr;

	ID3D11Texture2D* _pDepthBuffer = nullptr;
	ID3D11DepthStencilView* _pDepthBufferDSV = nullptr;
	ID3D11DepthStencilState* _pDepthState = nullptr;
	ID3D11DepthStencilState* _pSkyboxDepthState = nullptr;
	  
	ID3D11BlendState* _pBlendState = nullptr;

	Camera* _pCamera = nullptr;
	ID3D11SamplerState* _pSampler = nullptr;


	bool _mouseButtonPressed = false;
	POINT _prevMousePos;

	UINT _numSphereTriangles = 0.0;
	float _radius = 1.0;

	HRESULT _setupBackBuffer();
	HRESULT _setupDepthBuffer();
	HRESULT _initScene();
	bool _updateScene();
};