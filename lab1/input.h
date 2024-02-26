#pragma once
#include <d3d11_1.h>
#include <directxmath.h>
#include <dinput.h>

using namespace DirectX;

class Input 
{
public:
    Input();
    HRESULT Init(HINSTANCE hinstance, HWND hwnd);
    void Realese();
    XMFLOAT3 ReadInput();
    ~Input();
private:
    IDirectInput8* _directInput;
    IDirectInputDevice8* _mouse;

    DIMOUSESTATE _mouseState = {};
};