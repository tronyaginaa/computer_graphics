#include "camera.h"

Camera::Camera() 
{
    _center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    _r = 5.0f;
    _theta = XM_PIDIV4;
    _phi = -XM_PIDIV4;
    _updateViewMatrix();
}

void Camera::ChangePos(float dphi, float dtheta, float dr) 
{
    _phi -= dphi;
    _theta += dtheta;
    _theta = min(max(_theta, -XM_PIDIV2), XM_PIDIV2);
    _r += dr;
    if (_r < 2.0f) 
        _r = 2.0f;
    _updateViewMatrix();
}

void Camera::_updateViewMatrix() 
{
    XMFLOAT3 pos = XMFLOAT3(cosf(_theta) * cosf(_phi), sinf(_theta), cosf(_theta) * sinf(_phi));
    pos.x = pos.x * _r + _center.x;
    pos.y = pos.y * _r + _center.y;
    pos.z = pos.z * _r + _center.z;
    float upTheta = _theta + XM_PIDIV2;
    XMFLOAT3 up = XMFLOAT3(cosf(upTheta) * cosf(_phi), sinf(upTheta), cosf(upTheta) * sinf(_phi));

    _viewMatrix = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.0f),
        DirectX::XMVectorSet(_center.x, _center.y, _center.z, 0.0f),
        DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f)
    );
}