#pragma once

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>
#include "renderer.h"
#include "camera.h"
#include "input.h"
#include <DirectXMath.h>

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
	COLORREF color;
};

class Renderer 
{
public:
	void Render();
	void CleanupDevice();
	bool WinResize(UINT width, UINT height);
	HRESULT InitDevice(HINSTANCE hInstance, HWND hWnd);
	IDXGISwapChain* pSwapChain = nullptr;
private:
	D3D_DRIVER_TYPE         _driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL       _featureLevel = D3D_FEATURE_LEVEL_11_0;

	ID3D11Device* _pd3dDevice = nullptr;
	ID3D11Device1* _pd3dDevice1 = nullptr;

	ID3D11DeviceContext* _pImmediateContext = nullptr;
	ID3D11DeviceContext1* _pImmediateContext1 = nullptr;
	
	IDXGISwapChain1* _pSwapChain1 = nullptr;

	ID3D11RenderTargetView* _pRenderTargetView = nullptr;

	ID3D11Buffer* _pIndexBuffer = nullptr;
	ID3D11Buffer* _pVertexBuffer = nullptr;

	ID3D11VertexShader* _pVertexShader = nullptr;
	ID3D11PixelShader* _pPixelShader = nullptr;

	ID3D11InputLayout* _pInputLayout = nullptr;

	UINT _width;
	UINT _height;

	ID3D11Buffer* _pWorld = nullptr;
	ID3D11Buffer* _pView = nullptr;
	ID3D11RasterizerState* _pRasterizerState = nullptr;

	Camera* _pCamera = nullptr;
	Input* _pInput = nullptr;


	HRESULT _setupBackBuffer();
	HRESULT _initScene();
	void _inputHandler();
	bool _updateScene();
};