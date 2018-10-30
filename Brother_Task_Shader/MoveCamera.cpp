#include "MoveCamera.h"
#include <DirectXMath.h>
#include "Utils.h"
#include "math.h"

using namespace DirectX;

#define WASDMOVESPEED 0.02
#define UPDOWNMOVESPEED 0.01

double CameraLength = 0;

void MoveCamera(DIMOUSESTATE *MouseState, D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad) {

	CameraLength = CalcLength(CameraAt, CameraEye);

	//キーボードの入力を取得
	BYTE key[256];
	GetKeyboardState(key);

	//上下移動
	if(key[VK_SHIFT] & 0x80) {
		CameraAt->y += UPDOWNMOVESPEED;
		CameraEye->y += UPDOWNMOVESPEED;
	}
	else if (key[VK_CONTROL] & 0x80) {
		CameraAt->y -= UPDOWNMOVESPEED;
		CameraEye->y -= UPDOWNMOVESPEED;
	}

	//前後移動
	if (key['W'] & 0x80) {
		CameraAt->x += WASDMOVESPEED * cos(CameraRad->x);
		CameraEye->x += WASDMOVESPEED * cos(CameraRad->x);
		CameraAt->z += WASDMOVESPEED * sin(CameraRad->x);
		CameraEye->z += WASDMOVESPEED * sin(CameraRad->x);
	}
	else if (key['S'] & 0x80) {
		CameraAt->x += WASDMOVESPEED * cos(CameraRad->x + XM_PI);
		CameraEye->x += WASDMOVESPEED * cos(CameraRad->x + XM_PI);
		CameraAt->z += WASDMOVESPEED * sin(CameraRad->x + XM_PI);
		CameraEye->z += WASDMOVESPEED * sin(CameraRad->x + XM_PI);
	}

	//左右移動
	if (key['A'] & 0x80) {
		CameraAt->x += WASDMOVESPEED * cos(CameraRad->x + XM_PIDIV2);
		CameraEye->x += WASDMOVESPEED * cos(CameraRad->x + XM_PIDIV2);
		CameraAt->z += WASDMOVESPEED * sin(CameraRad->x + XM_PIDIV2);
		CameraEye->z += WASDMOVESPEED * sin(CameraRad->x + XM_PIDIV2);
	}
	else if (key['D'] & 0x80) {
		CameraAt->x += WASDMOVESPEED * cos(CameraRad->x - XM_PIDIV2);
		CameraEye->x += WASDMOVESPEED * cos(CameraRad->x - XM_PIDIV2);
		CameraAt->z += WASDMOVESPEED * sin(CameraRad->x - XM_PIDIV2);
		CameraEye->z += WASDMOVESPEED * sin(CameraRad->x - XM_PIDIV2);
	}

	//視点を原点にリセット
	if (key['R'] & 0x80) {
		
		D3DXVECTOR3 Root(0.0, 0.0, 0.0);

		ResetCamera(&Root, CameraEye, CameraRad);
		CalcCameraAt(CameraAt, CameraEye, CameraRad);

		return;
		
	}

	//マウスで視点操作
	if (MouseState->rgbButtons[0] != 0) {
		CameraRad->x -= (float)MouseState->lX * 0.001;
		CameraRad->y += (float)MouseState->lY * 0.001;

		if (CameraRad->x > XM_2PI) {
			CameraRad->x -= XM_2PI;
		}
		if (CameraRad->x < 0.0) {
			CameraRad->x += XM_2PI;
		}
		if (CameraRad->y > XM_2PI) {
			CameraRad->y -= XM_2PI;
		}
		if (CameraRad->y < 0.0) {
			CameraRad->y += XM_2PI;
		}

		CalcCameraAt(CameraAt, CameraEye, CameraRad);

		return;
	}

	return;

}

void CalcCameraAt(D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad) {

	CameraAt->x = CameraEye->x + CameraLength * sinf(CameraRad->y) * cosf(CameraRad->x);
	CameraAt->y = CameraEye->y + CameraLength * cosf(CameraRad->y);
	CameraAt->z = CameraEye->z + CameraLength * sinf(CameraRad->y) * sinf(CameraRad->x);	

}

void ResetCamera(D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad) {

	CameraLength = CalcLength(CameraAt, CameraEye);

	CameraRad->y = acosf((CameraAt->y - CameraEye->y) / CameraLength);
	CameraRad->x = acosf((CameraAt->x - CameraEye->x) / CameraLength / sinf(CameraRad->y));

}