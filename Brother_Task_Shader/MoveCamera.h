#pragma once

#include <dinput.h>
#include <d3dx10.h>

//���_�ƌ��Ă���p�x���璍���_���Z�o����
void CalcCameraAt(D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad);
//�J�����ړ��v�Z�̓���
void MoveCamera(DIMOUSESTATE *MouseState, D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad);
//�J�����̈ʒu�����_�Ƀ��Z�b�g
void ResetCamera(D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad);