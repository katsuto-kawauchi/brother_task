#pragma once

#include <d3dx10.h>

//2“_ŠÔ‚Ì‹——£‚ðŽZo
double CalcLength(D3DXVECTOR3 *VecA, D3DXVECTOR3 *VecB) {

	double answer = 0.0;

	answer = sqrt( pow(VecA->x - VecB->x, 2) + pow(VecA->y - VecB->y, 2) + pow(VecA->z - VecB->z, 2));

	return answer;

}
