#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include <vector>
#include "DDSTextureLoader.h"

using namespace DirectX;

struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
	XMFLOAT2 TexC;
};

struct SimpleVertexNormal
{
	XMFLOAT3 Pos;
	XMFLOAT4 normal;
	XMFLOAT2 TexC;
};

struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;

	//describes the colour of tthe object
	XMFLOAT4 DiffuseMtrl;
	//describes how much of each colour is reflected of the object
	XMFLOAT4 DiffuseLight;
	//vector that points in the direction of a light source
	XMFLOAT3 LightVecW;

	XMFLOAT4 AmbientMtrl;
	XMFLOAT4 AmbientLight;

	XMFLOAT4 SpecularMtrl;
	XMFLOAT4 SpecularLight;
	float SpecularPower;
	XMFLOAT3 EyePosW;
};

struct VertexType
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

using namespace std;
class Application
{
private:
	HINSTANCE               _hInst;
	HWND                    _hWnd;
	D3D_DRIVER_TYPE         _driverType;
	D3D_FEATURE_LEVEL       _featureLevel;
	ID3D11Device*           _pd3dDevice;
	ID3D11DeviceContext*    _pImmediateContext;
	IDXGISwapChain*         _pSwapChain;
	ID3D11RenderTargetView* _pRenderTargetView;
	ID3D11VertexShader*     _pVertexShader;
	ID3D11PixelShader*      _pPixelShader;
	ID3D11InputLayout*      _pVertexLayout;
	ID3D11Buffer            *_pVertexBuffer, *_pPyramidVertexBuffer, *_pGroundPlaneVertexBuffer;
	ID3D11Buffer            *_pIndexBuffer, *_pPyramidIndexBuffer, *_pGroundPlaneIndexBuffer;
	ID3D11Buffer*           _pConstantBuffer;
	XMFLOAT4X4              _world;
	XMFLOAT4X4              _view;
	XMFLOAT4X4              _projection;
	vector<XMFLOAT4X4>      _worldMatrices;
	XMFLOAT4X4              _pyramidWorldMatrix, _groundPlaneMatrix;
	ID3D11DepthStencilView* _depthStencilView;
	ID3D11Texture2D*        _depthStencilBuffer;
	ID3D11RasterizerState*  _wireFrame;
	ID3D11RasterizerState*  _solidObj;
	ID3D11ShaderResourceView* _pTextureRV;
	ID3D11SamplerState* _pSamplerLinear;
private:
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	HRESULT InitDevice();
	void Cleanup();
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitShadersAndInputLayout();
	HRESULT InitVertexBuffer();
	HRESULT InitIndexBuffer();
	HRESULT InitPlaneIndexBuffer(int vWidth, int vHeight);
	HRESULT InitPlaneVertexBuffer(UINT width, UINT depth, UINT Wverts, UINT Dverts);

	UINT _WindowHeight;
	UINT _WindowWidth;

	//XMFLOAT3 lightDirection;
	//XMFLOAT4 diffuseMaterial;
	//XMFLOAT4 diffuseLight;
public:
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	void Update();
	void Draw();
};

