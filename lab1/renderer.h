#pragma once

#include <d3d11_1.h>
#include <directxcolors.h>
#include <d3dcompiler.h>
#include "renderer.h"

#define SAFE_RELEASE(p) \
if (p != NULL) { \
    p->Release(); \
    p = NULL;\
}

struct Vertex {
	float x, y, z;
	COLORREF color;
};

class Renderer {
public:
	void Render();
	void CleanupDevice();
	bool WinResize(UINT width, UINT height);
	HRESULT InitDevice(HWND hWnd);
private:
	D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;

	ID3D11Device* g_pd3dDevice = nullptr;
	ID3D11Device1* g_pd3dDevice1 = nullptr;

	ID3D11DeviceContext* g_pImmediateContext = nullptr;
	ID3D11DeviceContext1* g_pImmediateContext1 = nullptr;

	IDXGISwapChain* g_pSwapChain = nullptr;
	IDXGISwapChain1* g_pSwapChain1 = nullptr;

	ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

	ID3D11Buffer* g_pIndexBuffer = nullptr;
	ID3D11Buffer* g_pVertexBuffer = nullptr;

	ID3D11VertexShader* g_pVertexShader = nullptr;
	ID3D11PixelShader* g_pPixelShader = nullptr;

	ID3D11InputLayout* g_pInputLayout = nullptr;

	UINT g_width;
	UINT g_height;

	HRESULT _setupBackBuffer();
	HRESULT _initScene();
};