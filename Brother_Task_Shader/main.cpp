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

//���S�ɉ������
#define SAFE_RELEASE(x) if(x){x->Release(); x=NULL;}

//�萔��`
#define WINDOW_WIDTH 1280 //�E�B���h�E��
#define WINDOW_HEIGHT 720 //�E�B���h�E����

#define DIRECTINPUT_VERSION		0x0800	

//�O���[�o���ϐ�
HWND hWnd = NULL;
HWND ActiveWnd = NULL;
ID3D11Device* Device = NULL;      //�f�o�C�X
ID3D11DeviceContext* DeviceContext = NULL;   //�f�o�C�X�R���e�L�X�g
IDXGISwapChain* SwapChain = NULL;     //�X���b�v�`�F�C��
ID3D11RenderTargetView* RenderTargetView = NULL; //�����_�[�^�[�Q�b�g�r���[
ID3D11DepthStencilView* DepthStencilView = NULL;
ID3D11Texture2D*  DepthStencil = NULL;
ID3D11InputLayout* VertexLayout = NULL;

ID3D11VertexShader* VertexShader = NULL;//���_�V�F�[�_�[
ID3D11PixelShader* PixelShader = NULL;//�s�N�Z���V�F�[�_�[

ID3D11Buffer* ConstantBuffer[2];//�R���X�^���g�o�b�t�@

D3DXVECTOR3 LightDir(1, 1, -1);//���C�g����
D3DXVECTOR3 CameraEye(0.0, 0.0, -200.0);//�J�������_
D3DXVECTOR3 CameraAt(0.0, 0.0, 0.0);//�J���������_
D3DXVECTOR2 CameraRad(XM_PI / 2, XM_PI / 2);//�J�����A���O��

ID3D11SamplerState* SampleLinear = NULL;//�e�N�X�`���[�̃T���v���[
ID3D11ShaderResourceView* Texture = NULL;//�e�N�X�`���[

LPDIRECTINPUT8       g_pDInput = NULL;	// DirectInput�I�u�W�F�N�g
LPDIRECTINPUTDEVICE8 g_pDIMouse = NULL;	// �}�E�X�f�o�C�X
DIMOUSESTATE g_zdiMouseState;

//�V�F�[�_�[�ɓn���l
struct SHADER_GLOBAL0
{
	D3DXMATRIX mW;//���[���h�s��
	D3DXMATRIX mWVP;//���[���h����ˉe�܂ł̕ϊ��s��
	D3DXVECTOR4 vLightDir;//���C�g����
	D3DXVECTOR4 vEye;//�J�����ʒu
};
struct SHADER_GLOBAL1
{
	D3DXVECTOR4 vAmbient;//�A���r�G���g
	D3DXVECTOR4 vDiffuse;//�f�B�t���[�Y
	D3DXVECTOR4 vSpecular;//���ʔ���
};

//�}�e���A���\����
struct MATERIAL
{
	CHAR Name[255];
	D3DXVECTOR4 Ka;//�A���r�G���g
	D3DXVECTOR4 Kd;//�f�B�t���[�Y
	D3DXVECTOR4 Ks;//�X�y�L�����[
	CHAR TextureName[255];//�e�N�X�`���t�@�C����
	ID3D11ShaderResourceView* pTexture;//�e�N�X�`���f�[�^
	DWORD MaxFace;//�|���S����
}mat;

//���_�\����
struct VERTEX
{
	D3DXVECTOR3 Pos;//�ʒu
	D3DXVECTOR3 Normal;//�@��
	D3DXVECTOR2 UV;//UV
}vert;

//.OBJ�t�@�C���N���X
class OBJ {
protected:
	vector <MATERIAL> Material;//�}�e���A��
	vector <VERTEX> Vertex;//���_�\����
	ID3D11Buffer* VertexBuffer;//���_�o�b�t�@
	ID3D11Buffer** IndexBuffer;//�C���f�b�N�X�o�b�t�@
	HRESULT LoadMaterialFromFile(LPSTR FileName);//�}�e���A���t�@�C���ǂݍ���
public:
	void Draw();//�`��
	HRESULT Load(LPSTR FileName);//�ǂݍ���
};

OBJ obj;

//���b�V���̕`��
void OBJ::Draw()
{
	//�o�[�e�b�N�X�o�b�t�@���Z�b�g
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);
	//�}�e���A���̐������A���ꂼ��̃}�e���A���̃C���f�b�N�X�o�b�t�@��`��
	for (DWORD i = 0; i < Material.size(); i++)
	{
		//�g�p����Ă��Ȃ��}�e���A���΍�
		if (Material[i].MaxFace == 0)
		{
			continue;
		}
		//�C���f�b�N�X�o�b�t�@���Z�b�g
		DeviceContext->IASetIndexBuffer(IndexBuffer[i], DXGI_FORMAT_R32_UINT, 0);
		//�v���~�e�B�u�E�g�|���W�[���Z�b�g
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//�}�e���A���̊e�v�f���V�F�[�_�[�ɓn��
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
		//�e�N�X�`�����V�F�[�_�[�ɓn��
		if (Material[i].TextureName[0] != NULL)
		{
			DeviceContext->PSSetSamplers(0, 1, &SampleLinear);
			DeviceContext->PSSetShaderResources(0, 1, &Material[i].pTexture);
		}
		DeviceContext->DrawIndexed(Material[i].MaxFace * 3, 0, 0);
	}
}

//�}�e���A���t�@�C����ǂݍ��ފ֐�
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
		//�L�[���[�h�ǂݍ���
		fscanf_s(fp, "%s ", key, sizeof(key));
		//�}�e���A����
		if (strcmp(key, "newmtl") == 0)
		{
			if (flag)Material.push_back(mat);
			flag = true;
			fscanf_s(fp, "%s ", key, sizeof(key));
			strcpy_s(mat.Name, key);
		}
		//Ka�@�A���r�G���g
		if (strcmp(key, "Ka") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			mat.Ka = v;
		}
		//Kd�@�f�B�t���[�Y
		if (strcmp(key, "Kd") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			mat.Kd = v;
		}
		//Ks�@�X�y�L�����[
		if (strcmp(key, "Ks") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			mat.Ks = v;
		}
		//map_Kd�@�e�N�X�`��
		if (strcmp(key, "map_Kd") == 0)
		{
			fscanf_s(fp, "%s", &mat.TextureName, sizeof(mat.TextureName));
			//�e�N�X�`�����쐬
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

//OBJ�t�@�C�����烁�b�V���ɕK�v�ȏ���ǂݍ���
HRESULT OBJ::Load(LPSTR FileName)
{
	//�ꎞ����p
	D3DXVECTOR3 vec3d;
	D3DXVECTOR2 vec2d;
	vector <D3DXVECTOR3> Pos;
	vector <D3DXVECTOR3> Normal;
	vector <D3DXVECTOR2> UV;
	vector <int> FaceIndex;
	int v1 = 0, v2 = 0, v3 = 0;
	int vn1 = 0, vn2 = 0, vn3 = 0;
	int vt1 = 0, vt2 = 0, vt3 = 0;
	DWORD dwFCount = 0;//�ǂݍ��݃J�E���^

	char key[255] = { 0 };
	//OBJ�t�@�C�����J���ē��e��ǂݍ���
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");

	//�ǂݍ��� 
	fseek(fp, SEEK_SET, 0);

	while (!feof(fp))
	{
		//�L�[���[�h
		ZeroMemory(key, sizeof(key));
		fscanf_s(fp, "%s ", key, sizeof(key));
		//�}�e���A��
		if (strcmp(key, "mtllib") == 0)
		{
			fscanf_s(fp, "%s ", key, sizeof(key));
			LoadMaterialFromFile(key);
		}
		//���_
		if (strcmp(key, "v") == 0)
		{
			fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
			Pos.push_back(vec3d);
			Vertex.push_back(vert);
		}
		//�@��
		if (strcmp(key, "vn") == 0)
		{
			fscanf_s(fp, "%f %f %f", &vec3d.x, &vec3d.y, &vec3d.z);
			Normal.push_back(vec3d);
		}
		//�e�N�X�`��
		if (strcmp(key, "vt") == 0)
		{
			fscanf_s(fp, "%f %f", &vec2d.x, &vec2d.y);
			UV.push_back(-vec2d);
		}
		//�t�F�C�X
		if (strcmp(key, "f") == 0)
		{
			FaceIndex.push_back(0);
			FaceIndex.push_back(0);
			FaceIndex.push_back(0);
		}
	}

	//�}�e���A���̐������C���f�b�N�X�o�b�t�@���쐬
	IndexBuffer = new ID3D11Buffer*[Material.size()];

	bool boFlag = false;
	for (DWORD i = 0; i < Material.size(); i++)
	{
		fseek(fp, SEEK_SET, 0);
		dwFCount = 0;

		while (!feof(fp))
		{
			//�L�[���[�h
			ZeroMemory(key, sizeof(key));
			fscanf_s(fp, "%s ", key, sizeof(key));

			//�t�F�C�X �ǂݍ��݁����_�C���f�b�N�X��
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

				//if (Material[i].pTexture != NULL)//�e�N�X�`���[����T�[�t�F�C�X
				//{
				//	fscanf_s(fp, "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				//}
				//else//�e�N�X�`���[�����T�[�t�F�C�X
				//{
				//	fscanf_s(fp, "%d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
				//}
				FaceIndex[dwFCount * 3] = v1 - 1;
				FaceIndex[dwFCount * 3 + 1] = v2 - 1;
				FaceIndex[dwFCount * 3 + 2] = v3 - 1;
				dwFCount++;
				//���_�\���̂ɑ��
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
		if (dwFCount == 0)//�g�p����Ă��Ȃ��}�e���A���΍�
		{
			IndexBuffer[i] = NULL;
			continue;
		}

		//�C���f�b�N�X�o�b�t�@���쐬
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

	//�o�[�e�b�N�X�o�b�t�@���쐬
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

//Direct3D�̏������֐�
HRESULT InitD3D(HWND hWnd)
{
	// �f�o�C�X�ƃX���b�v�`�F�[���̍쐬
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;         //�o�b�N�o�b�t�@�̐�
	sd.BufferDesc.Width = WINDOW_WIDTH;     //�o�b�t�@�̕�
	sd.BufferDesc.Height = WINDOW_HEIGHT;    //�o�b�t�@�̍���
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //�o�b�t�@�̃t�H�[�}�b�g
	sd.BufferDesc.RefreshRate.Numerator = 60;   //���t���b�V�����[�g
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
	//�����_�[�^�[�Q�b�g�r���[�̍쐬
	ID3D11Texture2D *BackBuffer;
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
	Device->CreateRenderTargetView(BackBuffer, NULL, &RenderTargetView);
	SAFE_RELEASE(BackBuffer);
	//�[�x�X�e���V���r���[�̍쐬
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
	//�����_�[�^�[�Q�b�g�r���[�Ɛ[�x�X�e���V���r���[���p�C�v���C���Ƀo�C���h 
	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);

	//�r���[�|�[�g�̐ݒ�
	D3D11_VIEWPORT vp;
	vp.Width = WINDOW_WIDTH;
	vp.Height = WINDOW_HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DeviceContext->RSSetViewports(1, &vp);

	//hlsl�t�@�C���ǂݍ���
	ID3DBlob *pCompiledShader = NULL;
	ID3DBlob *pErrors = NULL;
	//�u���u���璸�_�V�F�[�_�[�쐬
	if (FAILED(D3DX11CompileFromFile("shader.hlsl", NULL, NULL, "VS", "vs_5_0", 0, 0, NULL, &pCompiledShader, &pErrors, NULL)))
	{
		MessageBox(0, "���_�V�F�[�_�[�ǂݍ��ݎ��s", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pErrors);

	if (FAILED(Device->CreateVertexShader(pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), NULL, &VertexShader)))
	{
		SAFE_RELEASE(pCompiledShader);
		MessageBox(0, "���_�V�F�[�_�[�쐬���s", NULL, MB_OK);
		return E_FAIL;
	}
	//���_�C���v�b�g���C�A�E�g���` 
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
	 { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	 { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	 { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = sizeof(layout) / sizeof(layout[0]);

	//���_�C���v�b�g���C�A�E�g���쐬
	if (FAILED(Device->CreateInputLayout(layout, numElements, pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), &VertexLayout)))
		return FALSE;
	//���_�C���v�b�g���C�A�E�g���Z�b�g
	DeviceContext->IASetInputLayout(VertexLayout);

	//�u���u����s�N�Z���V�F�[�_�[�쐬
	if (FAILED(D3DX11CompileFromFile("shader.hlsl", NULL, NULL, "PS", "ps_5_0", 0, 0, NULL, &pCompiledShader, &pErrors, NULL)))
	{
		MessageBox(0, "�s�N�Z���V�F�[�_�[�ǂݍ��ݎ��s", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pErrors);
	if (FAILED(Device->CreatePixelShader(pCompiledShader->GetBufferPointer(), pCompiledShader->GetBufferSize(), NULL, &PixelShader)))
	{
		SAFE_RELEASE(pCompiledShader);
		MessageBox(0, "�s�N�Z���V�F�[�_�[�쐬���s", NULL, MB_OK);
		return E_FAIL;
	}
	SAFE_RELEASE(pCompiledShader);

	//���X�^���C�Y�ݒ�
	D3D11_RASTERIZER_DESC rdc;
	ZeroMemory(&rdc, sizeof(rdc));
	rdc.CullMode = D3D11_CULL_NONE;
	rdc.FillMode = D3D11_FILL_SOLID;

	ID3D11RasterizerState* pIr = NULL;
	Device->CreateRasterizerState(&rdc, &pIr);
	DeviceContext->RSSetState(pIr);
	SAFE_RELEASE(pIr);
	//�R���X�^���g�o�b�t�@�[�쐬�@�����ł͕ϊ��s��n���p
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
	//�R���X�^���g�o�b�t�@�[�쐬  �}�e���A���n���p
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
	//�e�N�X�`���[�p�T���v���[�쐬
	D3D11_SAMPLER_DESC SamDesc;
	ZeroMemory(&SamDesc, sizeof(D3D11_SAMPLER_DESC));

	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	Device->CreateSamplerState(&SamDesc, &SampleLinear);
	//OBJ�t�@�C������I���W�i�����b�V�����쐬
	obj.Load((LPSTR)FILE_NAME);

	return S_OK;
}

//�����_�����O
VOID Render()
{
	//�}�E�X�̏�Ԃ��擾
	HRESULT	hr = g_pDIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &g_zdiMouseState);

	float ClearColor[4] = { 0,0,1,1 }; // �N���A�F�쐬�@RGBA�̏�
	DeviceContext->ClearRenderTargetView(RenderTargetView, ClearColor);//��ʃN���A
	DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);//�[�x�o�b�t�@�N���A

	//�ϊ��s��쐬
	D3DXMATRIX mWorld;
	D3DXMATRIX mView;
	D3DXMATRIX mProj;

	//���[���h�s��
	static float angle = 0;
	//angle += 0.03f;
	D3DXMATRIX mRot;
	D3DXMatrixRotationY(&mRot, (float)D3DXToRadian(angle));
	mWorld = mRot;

	//�r���[�s�񑀍�
	MoveCamera(&g_zdiMouseState, &CameraAt, &CameraEye, &CameraRad);

	D3DXVECTOR3 Up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &CameraEye, &CameraAt, &Up);

	//�v���W�F�N�V�����s��
	D3DXMatrixPerspectiveFovLH(&mProj, (float)D3DX_PI / 4, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 1000.0f);

	//���[���h�E�r���[�E�v���W�F�N�V�����s����V�F�[�_�[�ɓn��
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
	//�g�p����V�F�[�_�[�̓o�^
	DeviceContext->VSSetShader(VertexShader, NULL, 0);
	DeviceContext->PSSetShader(PixelShader, NULL, 0);
	//���b�V����`��
	obj.Draw();

	ShowCursor(true);

	SwapChain->Present(0, 0);//�t���b�v
}

//�I�����������
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

//���b�Z�[�W�v���V�[�W��
LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY://�I����
		Cleanup();
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool InitMouse(HINSTANCE hInst) {
	//���͊֌W������
	HRESULT ret = DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_pDInput, NULL);
	if (FAILED(ret)) {
		return false;	// �쐬�Ɏ��s
	}

	if (g_pDInput == NULL) {
		return false;
	}

	// �}�E�X�p�Ƀf�o�C�X�I�u�W�F�N�g���쐬
	ret = g_pDInput->CreateDevice(GUID_SysMouse, &g_pDIMouse, NULL);
	if (FAILED(ret)) {
		// �f�o�C�X�̍쐬�Ɏ��s
		return false;
	}

	// �f�[�^�t�H�[�}�b�g��ݒ�
	ret = g_pDIMouse->SetDataFormat(&c_dfDIMouse);	// �}�E�X�p�̃f�[�^�E�t�H�[�}�b�g��ݒ�
	if (FAILED(ret)) {
		// �f�[�^�t�H�[�}�b�g�Ɏ��s
		return false;
	}

	// ���[�h��ݒ�i�t�H�A�O���E���h����r�����[�h�j
	ret = g_pDIMouse->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(ret)) {
		// ���[�h�̐ݒ�Ɏ��s
		return false;
	}

	// �f�o�C�X�̐ݒ�
	DIPROPDWORD diprop;
	diprop.diph.dwSize = sizeof(diprop);
	diprop.diph.dwHeaderSize = sizeof(diprop.diph);
	diprop.diph.dwObj = 0;
	diprop.diph.dwHow = DIPH_DEVICE;
	diprop.dwData = DIPROPAXISMODE_REL;	// ���Βl���[�h�Őݒ�i��Βl��DIPROPAXISMODE_ABS�j

	ret = g_pDIMouse->SetProperty(DIPROP_AXISMODE, &diprop.diph);
	if (FAILED(ret)) {
		// �f�o�C�X�̐ݒ�Ɏ��s
		return false;
	}

	// ���͐���J�n
	g_pDIMouse->Acquire();

	return true;

}

//���C���֐�
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR szStr, INT iCmdShow)
{
	//�E�C���h�E�N���X�̓o�^
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
					  GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					  "Window1", NULL };
	RegisterClassEx(&wc);
	//�^�C�g���o�[�ƃE�C���h�E�g�̕����܂߂ăE�C���h�E�T�C�Y��ݒ�
	RECT rect;
	SetRect(&rect, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	rect.right = rect.right - rect.left;
	rect.bottom = rect.bottom - rect.top;
	rect.top = 0;
	rect.left = 0;
	//�E�C���h�E�̐���
	hWnd = CreateWindow("Window1", "OBJ���[�_",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right, rect.bottom,
		NULL, NULL, wc.hInstance, NULL);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	//Direct3D������
	if (!SUCCEEDED(InitD3D(hWnd))) {
		return false;
	}
	
	//�E�C���h�E�\��
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

		//Esc�L�[�������ꂽ��I��
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