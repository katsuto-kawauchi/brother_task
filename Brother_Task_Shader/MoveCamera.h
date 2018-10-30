#pragma once

#include <dinput.h>
#include <d3dx10.h>

//視点と見ている角度から注視点を算出する
void CalcCameraAt(D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad);
//カメラ移動計算の入口
void MoveCamera(DIMOUSESTATE *MouseState, D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad);
//カメラの位置を原点にリセット
void ResetCamera(D3DXVECTOR3 *CameraAt, D3DXVECTOR3 *CameraEye, D3DXVECTOR2 *CameraRad);