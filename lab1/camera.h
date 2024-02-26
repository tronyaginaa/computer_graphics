#pragma once
#include <d3d11_1.h>
#include <directxmath.h>

using namespace DirectX;

class Camera 
{
public:
    Camera();
    void ChangePos(float dphi, float dtheta, float dr);

    XMMATRIX& GetViewMatrix() { return _viewMatrix; };
private:
    XMMATRIX _viewMatrix;
    XMFLOAT3 _center;
    float _r;
    float _theta;
    float _phi;
    void _updateViewMatrix();
};