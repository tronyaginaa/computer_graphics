#include "input.h"

Input::Input() : _directInput(NULL), _mouse(NULL) {}

HRESULT Input::Init(HINSTANCE hinstance, HWND hwnd) 
{
    HRESULT result;

    result = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&_directInput, NULL);

    if (SUCCEEDED(result)) 
    {
        result = _directInput->CreateDevice(GUID_SysMouse, &_mouse, NULL);
    }

    if (SUCCEEDED(result)) 
    {
        result = _mouse->SetDataFormat(&c_dfDIMouse);
    }

    if (SUCCEEDED(result)) 
    {
        result = _mouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    }

    if (SUCCEEDED(result)) 
    {
        result = _mouse->Acquire();
    }

    return result;
}

XMFLOAT3 Input::ReadInput() 
{
    HRESULT result;

    result = _mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&_mouseState);
    if (FAILED(result)) 
    {
        if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
            _mouse->Acquire();
    }
    else 
    {
        if (_mouseState.rgbButtons[0] || _mouseState.rgbButtons[1] || _mouseState.rgbButtons[2] & 0x80)
            return XMFLOAT3((float)_mouseState.lX, (float)_mouseState.lY, (float)_mouseState.lZ);
    }

    return XMFLOAT3(0.0f, 0.0f, 0.0f);
}

Input::~Input() 
{
    if (_mouse) 
    {
        _mouse->Unacquire();
        _mouse->Release();
        _mouse = NULL;
    }

    if (_directInput) 
    {
        _directInput->Release();
        _directInput = NULL;
    }
}