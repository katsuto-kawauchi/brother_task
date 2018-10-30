#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dx11.lib")
#pragma comment(lib,"d3dx10.lib")
#pragma comment(lib,"d3dCompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#include <stdio.h>
#include <vector>
#include <d3dx11.h>
#include <d3dx10.h>
#include <DirectXMath.h>
#include <d3dCompiler.h>
#include <dinput.h>
#include <Windows.h>
#include "MoveCamera.h"

#define FILE_NAME "sword.obj"

using namespace std;
using namespace DirectX;

//安全に解放する
#define SAFE_RELEASE(x) if(x){x->Release(); x=NULL;}

//定数定義
#define WINDOW_WIDTH 1280 //ウィンドウ幅
#define WINDOW_HEIGHT 720 //ウィンドウ高さ

#define DIRECTINPUT_VERSION		0x0800	

//グローバル変数
HWND hWnd = NULL;
HWND ActiveWnd = NULL;
ID3D11Device* Device = NULL;      //デバイス
ID3D11DeviceContext* DeviceContext = NULL;   //デバイスコンテキスト
IDXGISwapChain* SwapChain = NULL;     //スワップチェイン
ID3D11RenderTargetView* RenderTargetView = NULL; //レンダーターゲットビュー
ID3D11DepthStencilView* DepthStencilView = NULL;
ID3D11Texture2D*  DepthStencil = NULL;
ID3D11InputLayout* VertexLayout = NULL;

ID3D11VertexShader* VertexShader = NULL;//頂点シェーダー
ID3D11PixelShader* PixelShader = NULL;//ピクセルシェーダー

ID3D11Buffer* ConstantBuffer[2];//コンスタントバッファ

D3DXVECTOR3 LightDir(1, 1, -1);//ライト方向
D3DXVECTOR3 CameraEye(0.0, 0.0, -200.0);//カメラ視点
D3DXVECTOR3 CameraAt(0.0, 0.0, 0.0);//カメラ注視点
D3DXVECTOR2 CameraRad(XM_PI / 2, XM_PI / 2);//カメラアングル

ID3D11SamplerState* SampleLinear = NULL;//テクスチャーのサンプラー
ID3D11ShaderResourceView* Texture = NULL;//テクスチャー

LPDIRECTINPUT8       g_pDInput = NULL;	// DirectInputオブジェクト
LPDIRECTINPUTDEVICE8 g_pDIMouse = NULL;	// マウスデバイス
DIMOUSESTATE g_zdiMouseState;

//シェーダーに渡す値
struct SHADER_GLOBAL0
{
	D3DXMATRIX mW;//ワールド行列
	D3DXMATRIX mWVP;//ワールドから射影までの変換行列
	D3DXVECTOR4 vLightDir;//ライト方向
	D3DXVECTOR4 vEye;//カメラ位置
};
struct SHADER_GLOBAL1
{
	D3DXVECTOR4 vAmbient;//アンビエント
	D3DXVECTOR4 vDiffuse;//ディフューズ
	D3DXVECTOR4 vSpecular;//鏡面反射
};

//マテリアル構造体
struct MATERIAL
{
	CHAR Name[255];
	D3DXVECTOR4 Ka;//アンビエント
	D3DXVECTOR4 Kd;//ディフューズ
	D3DXVECTOR4 Ks;//スペキュラー
	CHAR TextureName[255];//テクスチャファイル名
	ID3D11ShaderResourceView* pTexture;//テクスチャデータ
	DWORD MaxFace;//ポリゴン数
}mat;

//頂点構造体
struct VERTEX
{
	D3DXVECTOR3 Pos;//位置
	D3DXVECTOR3 Normal;//法線
	D3DXVECTOR2 UV;//UV
}vert;

//.OBJファイルクラス
class OBJ {
protected:
	vector <MATERIAL> Material;//マテリアル
	vector <VERTEX> Vertex;//頂点構造体
	ID3D11Buffer* VertexBuffer;//頂点バッファ
	ID3D11Buffer** IndexBuffer;//インデックスバッファ
	HRESULT LoadMaterialFromFile(LPSTR FileName);//マテリアルファイル読み込み
public:
	void Draw();//描画
	HRESULT Load(LPSTR FileName);//読み込み
};

OBJ obj;

//メッシュの描画
void OBJ::Draw()
{
	//バーテックスバッファをセット
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	//マテリアルの数だけ、それぞれのマテリアルのインデックスバッファを描画
	for (DWORD i = 0; i < Material.size(); i++)
	{
		//使用されていないマテリアル対策
		if (Material[i].MaxFace == 0)
		{
			continue;
		}
		//インデックスバッファをセット
		DeviceContext->IASetIndexBuffer(IndexBuffer[i], DXGI_FORMAT_R32_UINT, 0);
		//プリミティブ・トポロジーをセット
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//マテリアルの各要素をシェーダーに渡す
		D3D11_MAPPED_SUBRESOURCE pData;
		if (SUCCEEDED(DeviceContext->Map(ConstantBuffer[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &pData)))
		{
			SHADER_GLOBAL1 sg;
			sg.vAmbient = Material[i].Ka;
			sg.vDiffuse = Material[i].Kd;
			sg.vSpecular = Material[i].Ks;
			memcpy_s(pData.pData, pData.RowPitch, (void*)&sg, sizeof(SHADER_GLOBAL1));
			DeviceContext->Unmap(ConstantBuffer[1], 0);
		}
		DeviceContext->VSSetConstantBuffers(1, 1, &ConstantBuffer[1]);
		DeviceContext->PSSetConstantBuffers(1, 1, &ConstantBuffer[1]);
		//テクスチャをシェーダーに渡す
		if (Material[i].TextureName[0] != NULL)
		{
			DeviceContext->PSSetSamplers(0, 1, &SampleLinear);
			DeviceContext->PSSetShaderResources(0, 1, &Material[i].pTexture);
		}
		DeviceContext->DrawIndexed(Material[i].MaxFace * 3, 0, 0);
	}
}

//マテリアルファイルを読み込む関数
HRESULT OBJ::LoadMaterialFromFile(LPSTR FileName)
{
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");
	char key[255] = { 0 };
	bool flag = false;
	D3DXVECTOR4 v(0, 0, 0, 0);

	fseek(fp, SEEK_SET, 0);

	while (!feof(fp))
	{
		//キーワード読み込み
		fscanf_s(fp, "%s ", key, sizeof(key));
		//マテリアル名
		if (strcmp(key, "newmtl") == 0)
		{
			if (flag)Material.push_back(mat);
			flag = true;
			fscanf_s(fp, "%s ", key, sizeof(key));
			strcpy_s(mat.Name, key);
		}
		//Ka　アンビエント
		if (strcmp(key, "Ka") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			mat.Ka = v;
		}
		//Kd　ディフューズ
		if (strcmp(key, "Kd") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			mat.Kd = v;
		}
		//Ks　スペキュラー
		if (strcmp(key, "Ks") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			mat.Ks = v;
		}
		//map_Kd　テクスチャ
		if (strcmp(key, "map_Kd") == 0)
		{
			fscanf_s(fp, "%s", &mat.TextureName, sizeof(mat.TextureName));
			//テクスチャを作成
			if (FAILED(D3DX11CreateShaderResourceViewFromFileA(Device, mat.TextureName, NULL, NULL, &mat.pTexture, NULL)))
			{
				return E_FAIL;
			}
		}
	}
	fclose(fp);
	if (flag)Material.push_back(mat);

	return S_OK;
}

//OBJファイルからメッシュに必要な情報を読み込む
HRESULT OBJ::Load(LPSTR FileName)
{
	//一時代入用
	D3DXVECTOR3 vec3d;
	D3DXVECTOR2 vec2d;
	vector <D3DXVECTOR3> Pos;
	vector <D3DXVECTOR3> Normal;
	vector <D3DXVECTOR2> UV;
	vector <int> FaceIndex;
	int v1 = 0, v2 = 0, v3 = 0;
	int vn1 = 0, vn2 = 0, vn3 = 0;
	int vt1 = 0, vt2 = 0, vt3 = 0;
	DWORD dwFCount = 0;//読み込みカウンタ

	char key[255] = { 0 };
	//OBJファイルを開いて内容を読み込む
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");

	//読み込み 
	fseek(fp, SEEK_SET, 0);

	while (!feof(fp))
	{
		//キーワード
		ZeroMemory(key, sizeof(key));
		fscanf_s(fp, "%s ", key, sizeof(key));
		//マテリアル
		if (strcmp(key, "mtllib") == 0)
		{
			fscanf_s(fp, "%s ", key, sizeof(key));
			LoadMaterialFromFile(key);
		}
		//頂点
		if (strcmp(key, "v") == 0)
		{
			fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
			Pos.push_back(vec3d);
			Vertex.push_back(vert);
		}
		//法線
		if (strcmp(key, "vn") == 0)
		{
			fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
			Normal.push_back(vec3d);
		}
		//テクスチャ
		if (strcmp(key, "vt") == 0)
		{
			fscanf_s(fp, "%f %f", &vec2d.x, &vec2d.y);
			UV.push_back(-vec2d);
		}
		//フェイス
		if (strcmp(key, "f") == 0)
		{
			FaceIndex.push_back(0);
			FaceIndex.push_back(0);
			FaceIndex.push_back(0);
		}
	}

	//マテリアルの数だけインデックスバッファを作成
	IndexBuffer = new ID3D11Buffer*[Material.size()];

	bool boFlag = false;
	for (DWORD i = 0; i < Material.size(); i++)
	{
		fseek(fp, SEEK_SET, 0);
		dwFCount = 0;

		while (!feof(fp))
		{
			//キーワード
			ZeroMemory(key, sizeof(key));
			fscanf_s(fp, "%s ", key, sizeof(key));

			//フェイス 読み込み→頂点インデックスに
			if (strcmp(key, "usemtl") == 0)
			{
				fscanf_s(fp, "%s ", key, sizeof(key));
				if (strcmp(key, Material[i].Name) == 0)
				{
					boFlag = true;
				}
				else
				{
					boFlag = false;
				}
			}
			if (strcmp(key, "f") == 0 && boFlag == true)
			{
				fscanf_s(fp, "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);

				//if (Material[i].pTexture != NULL)//テクスチャーありサーフェイス
				//{
				//	fscanf_s(fp, "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				//}
				//else//テクスチャー無しサーフェイス
				//{
				//	fscanf_s(fp, "%d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
				//}
				FaceIndex[dwFCount * 3] = v1 - 1;
				FaceIndex[dwFCount * 3 + 1] = v2 - 1;
				FaceIndex[dwFCount * 3 + 2] = v3 - 1;
				dwFCount++;
				//頂点構造体に代入
				Vertex[v1 - 1].Pos = Pos[v1 - 1];
				Vertex[v1 - 1].Normal = Normal[vn1 - 1];
				Vertex[v1 - 1].UV = UV[vt1 - 1];
				Vertex[v2 - 1].Pos = Pos[v2 - 1];
				Vertex[v2 - 1].Normal = Normal[vn2 - 1];
				Vertex[v2 - 1].UV = UV[vt2 - 1];
				Vertex[v3 - 1].Pos = Pos[v3 - 1];
				Vertex[v3 - 1].Normal = Normal[vn3 - 1];
				Vertex[v3 - 1].UV = UV[vt3 - 1];
			}
		}
		if (dwFCount == 0)//使用されていないマテリアル対策
		{
			IndexBuffer[i] = NULL;
			continue;
		}

		//インデックスバッファを作成
		D3D11_BUFFER_DESC bd;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(int) * dwFCount * 3;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = &FaceIndex[0];
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;
		if (FAILED(Device->CreateBuffer(&bd, &InitData, &IndexBuffer[i])))return FALSE;
		Material[i].MaxFace = dwFCount;
	}
	FaceIndex.clear();
	fclose(fp);

	//バーテックスバッファを作成
	D3D11_BUFFER_DESC bd;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(VERTEX) *Vertex.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &Vertex[0];
	if (FAILED(Device->CreateBuffer(&bd, &InitData, &VertexBuffer)))return FALSE;


	Pos.clear();
	Normal.clear();
	UV.clear();
	Vertex.clear();

	return S_OK;
}

//Direct3Dの初期化関数
HRESULT InitD3D(HWND hWnd)
{
	// デバイスとスワップチェーンの作成
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;         //バックバッファの数
	sd.BufferDesc.Width = WINDOW_WIDTH;     //バッファの幅
	sd.BufferDesc.Height = WINDOW_HEIGHT;    //バッファの高さ
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //バッファのフォーマット
	sd.BufferDesc.RefreshRate.Numerator = 60;   //リフレッシュレート
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	D3D_FEATURE_LEVEL  FeatureLevel = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL* pFeatureLevel = NULL;


	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
		&FeatureLevel, 1, D3D11_SDK_VERSION, &sd, &SwapChain, &Device, NULL, &DeviceContext)))
	{
		return FALSE;
	}
	//レンダーターゲットビューの作成
	ID3D11Texture2D *BackBuffer;
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
	Device->CreateRenderTargetView(BackBuffer, NULL, &RenderTargetView);
	SAFE_RELEASE(BackBuffer);
	//深度ステンシルビューの作成
	D3D11_TEXTURE2D_DESC descDepth;
	descDepth.Width = WINDOW_WIDTH;
	descDepth.Height = WINDOW_HEIGHT;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	Device->CreateTexture2D(&descDepth, NULL, &DepthStencil);

	Device->CreateDepthStencilView(DepthStencil, NULL, &DepthStencilView);
	//レンダーターゲットビューと深度ステンシルビューをパイプラインにバインド 
	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);

	//ビューポートの設定
	D3D11_VIEWPORT vp;
	vp.Width = WINDOW_WIDTH;
	vp.Height = WINDOW_HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DeviceContext->RSSetViewports(1, &vp);

	//hlslファイル読み込み
	ID3DBlob *pCompiledShader = NULL;
	ID3DBlob *pErrors = NULL;
	//ブロブから頂点シェーダー作成
	if (FAILED(D3DX11CompileFromFile("shader.hlsl", NULL, NULL, "VS", "vs_5_0", 0, 0, NULL, &pCompiledShader, &pErrors, NULL)))
	{
		MessageBox(0, "頂点シェーダー読み込み失敗", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pErrors);

	if (FAILED(Device->CreateVertexShader(pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), NULL, &VertexShader)))
	{
		SAFE_RELEASE(pCompiledShader);
		MessageBox(0, "頂点シェーダー作成失敗", NULL, MB_OK);
		return E_FAIL;
	}
	//頂点インプットレイアウトを定義 
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
	 { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	 { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	 { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = sizeof(layout) / sizeof(layout[0]);

	//頂点インプットレイアウトを作成
	if (FAILED(Device->CreateInputLayout(layout, numElements, pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), &VertexLayout)))
		return FALSE;
	//頂点インプットレイアウトをセット
	DeviceContext->IASetInputLayout(VertexLayout);

	//ブロブからピクセルシェーダー作成
	if (FAILED(D3DX11CompileFromFile("shader.hlsl", NULL, NULL, "PS", "ps_5_0", 0, 0, NULL, &pCompiledShader, &pErrors, NULL)))
	{
		MessageBox(0, "ピクセルシェーダー読み込み失敗", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pErrors);
	if (FAILED(Device->CreatePixelShader(pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), NULL, &PixelShader)))
	{
		SAFE_RELEASE(pCompiledShader);
		MessageBox(0, "ピクセルシェーダー作成失敗", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pCompiledShader);

	//ラスタライズ設定
	D3D11_RASTERIZER_DESC rdc;
	ZeroMemory(&rdc, sizeof(rdc));
	rdc.CullMode = D3D11_CULL_NONE;
	rdc.FillMode = D3D11_FILL_SOLID;

	ID3D11RasterizerState* pIr = NULL;
	Device->CreateRasterizerState(&rdc, &pIr);
	DeviceContext->RSSetState(pIr);
	SAFE_RELEASE(pIr);
	//コンスタントバッファー作成　ここでは変換行列渡し用
	D3D11_BUFFER_DESC cb;
	cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb.ByteWidth = sizeof(SHADER_GLOBAL0);
	cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb.MiscFlags = 0;
	cb.StructureByteStride = 0;
	cb.Usage = D3D11_USAGE_DYNAMIC;

	if (FAILED(Device->CreateBuffer(&cb, NULL, &ConstantBuffer[0])))
	{
		return E_FAIL;
	}
	//コンスタントバッファー作成  マテリアル渡し用
	cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb.ByteWidth = sizeof(SHADER_GLOBAL1);
	cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cb.MiscFlags = 0;
	cb.StructureByteStride = 0;
	cb.Usage = D3D11_USAGE_DYNAMIC;

	if (FAILED(Device->CreateBuffer(&cb, NULL, &ConstantBuffer[1])))
	{
		return E_FAIL;
	}
	//テクスチャー用サンプラー作成
	D3D11_SAMPLER_DESC SamDesc;
	ZeroMemory(&SamDesc, sizeof(D3D11_SAMPLER_DESC));

	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	Device->CreateSamplerState(&SamDesc, &SampleLinear);
	//OBJファイルからオリジナルメッシュを作成
	obj.Load((LPSTR)FILE_NAME);

	return S_OK;
}

//レンダリング
VOID Render()
{
	//マウスの状態を取得
	HRESULT	hr = g_pDIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &g_zdiMouseState);

	float ClearColor[4] = { 0,0,1,1 }; // クリア色作成　RGBAの順
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);//画面クリア
	DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);//深度バッファクリア

	//変換行列作成
	D3DXMATRIX mWorld;
	D3DXMATRIX mView;
	D3DXMATRIX mProj;

	//ワールド行列
	static float angle = 0;
	//angle += 0.03f;
	D3DXMATRIX mRot;
	D3DXMatrixRotationY(&mRot, (float)D3DXToRadian(angle));
	mWorld = mRot;

	//ビュー行列操作
	MoveCamera(&g_zdiMouseState, &CameraAt, &CameraEye, &CameraRad);

	D3DXVECTOR3 Up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &CameraEye, &CameraAt, &Up);

	//プロジェクション行列
	D3DXMatrixPerspectiveFovLH(&mProj, (float)D3DX_PI / 4, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 1000.0f);

	//ワールド・ビュー・プロジェクション行列をシェーダーに渡す
	DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer[0]);

	D3D11_MAPPED_SUBRESOURCE pData;
	if (SUCCEEDED(DeviceContext->Map(ConstantBuffer[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &pData)))
	{
		SHADER_GLOBAL0 sg;
		sg.mW = mWorld;
		D3DXMatrixTranspose(&sg.mW, &sg.mW);
		sg.mWVP = mWorld * mView*mProj;
		D3DXMatrixTranspose(&sg.mWVP, &sg.mWVP);
		sg.vLightDir = D3DXVECTOR4(LightDir.x, LightDir.y, LightDir.z, 0.0f);
		sg.vEye = D3DXVECTOR4(CameraEye.x, CameraEye.y, CameraEye.z, 0);
		memcpy_s(pData.pData, pData.RowPitch, (void*)&sg, sizeof(SHADER_GLOBAL0));
		DeviceContext->Unmap(ConstantBuffer[0], 0);
	}
	DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer[0]);
	DeviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer[0]);
	//使用するシェーダーの登録
	DeviceContext->VSSetShader(VertexShader, NULL, 0);
	DeviceContext->PSSetShader(PixelShader, NULL, 0);
	//メッシュを描画
	obj.Draw();

	ShowCursor(true);

	SwapChain->Present(0, 0);//フリップ
}

//終了時解放処理
VOID Cleanup()
{
	SAFE_RELEASE(SampleLinear);
	SAFE_RELEASE(Texture);
	SAFE_RELEASE(DepthStencil);
	SAFE_RELEASE(DepthStencilView);
	SAFE_RELEASE(VertexShader);
	SAFE_RELEASE(PixelShader);
	SAFE_RELEASE(ConstantBuffer[0]);
	SAFE_RELEASE(ConstantBuffer[1]);
	SAFE_RELEASE(VertexLayout);
	SAFE_RELEASE(SwapChain);
	SAFE_RELEASE(RenderTargetView);
	SAFE_RELEASE(DeviceContext);
	SAFE_RELEASE(Device);
	SAFE_RELEASE(g_pDInput);
	SAFE_RELEASE(g_pDIMouse);
}

//メッセージプロシージャ
LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY://終了時
		Cleanup();
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool InitMouse(HINSTANCE hInst) {
	//入力関係初期化
	HRESULT ret = DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_pDInput, NULL);
	if (FAILED(ret)) {
		return false;	// 作成に失敗
	}

	if (g_pDInput == NULL) {
		return false;
	}

	// マウス用にデバイスオブジェクトを作成
	ret = g_pDInput->CreateDevice(GUID_SysMouse, &g_pDIMouse, NULL);
	if (FAILED(ret)) {
		// デバイスの作成に失敗
		return false;
	}

	// データフォーマットを設定
	ret = g_pDIMouse->SetDataFormat(&c_dfDIMouse);	// マウス用のデータ・フォーマットを設定
	if (FAILED(ret)) {
		// データフォーマットに失敗
		return false;
	}

	// モードを設定（フォアグラウンド＆非排他モード）
	ret = g_pDIMouse->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(ret)) {
		// モードの設定に失敗
		return false;
	}

	// デバイスの設定
	DIPROPDWORD diprop;
	diprop.diph.dwSize = sizeof(diprop);
	diprop.diph.dwHeaderSize = sizeof(diprop.diph);
	diprop.diph.dwObj = 0;
	diprop.diph.dwHow = DIPH_DEVICE;
	diprop.dwData = DIPROPAXISMODE_REL;	// 相対値モードで設定（絶対値はDIPROPAXISMODE_ABS）

	ret = g_pDIMouse->SetProperty(DIPROP_AXISMODE, &diprop.diph);
	if (FAILED(ret)) {
		// デバイスの設定に失敗
		return false;
	}

	// 入力制御開始
	g_pDIMouse->Acquire();

	return true;

}

//メイン関数
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR szStr, INT iCmdShow)
{
	//ウインドウクラスの登録
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
					  GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					  "Window1", NULL };
	RegisterClassEx(&wc);
	//タイトルバーとウインドウ枠の分を含めてウインドウサイズを設定
	RECT rect;
	SetRect(&rect, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	rect.right = rect.right - rect.left;
	rect.bottom = rect.bottom - rect.top;
	rect.top = 0;
	rect.left = 0;
	//ウインドウの生成
	hWnd = CreateWindow("Window1", "OBJローダ",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right, rect.bottom,
		NULL, NULL, wc.hInstance, NULL);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	//Direct3D初期化
	if (!SUCCEEDED(InitD3D(hWnd))) {
		return false;
	}
	
	//ウインドウ表示
	ShowWindow(hWnd, SW_NORMAL);
	UpdateWindow(hWnd);
	while (msg.message != WM_QUIT)
	{
		bool nowActive = true;

		ActiveWnd = GetActiveWindow();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (g_pDInput == NULL) {
				InitMouse(hInst);
			}
			Render();
		}

		//Escキーが押されたら終了
		BYTE key[256];
		GetKeyboardState(key);
		if (key[VK_ESCAPE] & 0x80) {
			break;
		}

		if (hWnd == ActiveWnd) {

			if (nowActive == false) {
				if (InitMouse(hInst)) {
					nowActive = true;
				}
			}

		}
		else {

			SAFE_RELEASE(g_pDInput);
			SAFE_RELEASE(g_pDIMouse);

			nowActive = false;

		}

	}

}