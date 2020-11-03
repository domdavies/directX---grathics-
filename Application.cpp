#include "Application.h"

using namespace std;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

Application::Application()
{
    _hInst = nullptr;
    _hWnd = nullptr;
    _driverType = D3D_DRIVER_TYPE_NULL;
    _featureLevel = D3D_FEATURE_LEVEL_11_0;
    _pd3dDevice = nullptr;
    _pImmediateContext = nullptr;
    _pSwapChain = nullptr;
    _pRenderTargetView = nullptr;
    _pVertexShader = nullptr;
    _pPixelShader = nullptr;
    _pVertexLayout = nullptr;
    _pVertexBuffer = nullptr;
    _pPyramidVertexBuffer = nullptr;
    _pIndexBuffer = nullptr;
    _pConstantBuffer = nullptr;
    _pTextureRV = nullptr;
    _pSamplerLinear = nullptr;
}

Application::~Application()
{
	Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
    {
        return E_FAIL;
    }

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _WindowWidth = rc.right - rc.left;
    _WindowHeight = rc.bottom - rc.top;

    if (FAILED(InitDevice()))
    {
        Cleanup();

        return E_FAIL;
    }

    // Initialize the world matrix
    for (int i = 0; i < 5; i++)
    {
        XMFLOAT4X4 world;
        XMStoreFloat4x4(&world, XMMatrixIdentity());
        _worldMatrices.push_back(world);

    }
    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, 25.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMStoreFloat4x4(&_view, XMMatrixLookAtLH(Eye, At, Up));
    
    // Initialize the projection matrix
    XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, _WindowWidth / (FLOAT)_WindowHeight, 0.01f, 100.0f));
    CreateDDSTextureFromFile(_pd3dDevice, L"Crate_COLOR.dds", nullptr, &_pTextureRV);

	return S_OK;
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{	
		pVSBlob->Release();
        return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

    if (FAILED(hr))
        return hr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
     //   { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
     //   { "BITANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

    if (FAILED(hr))
        return hr;

    // Set the input layout
    _pImmediateContext->IASetInputLayout(_pVertexLayout);

	return hr;
}

HRESULT Application::InitVertexBuffer()
{
	HRESULT hr;

    // Create vertex buffer
    SimpleVertexNormal cubeVertices[] =
    {
        /*
        { XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT4(1, 1, 0, 0) },
        { XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT4(1, 1, 0, 0) },
        { XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT4(1, 1, 0, 0) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT4(1, 1, 0, 0) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1, 1, 0, 0)},
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1, 1, 0, 0) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(1, 1, 0, 0) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1, 1, 0, 0) }
        */
        //front face
        { XMFLOAT3(1, 1, 1), XMFLOAT4(0, 0, -1, 0), XMFLOAT2(1,0) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },
        { XMFLOAT3(-1, 1, 1), XMFLOAT4(0, 0, -1, 0), XMFLOAT2(0,0)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},
        { XMFLOAT3(-1, -1, 1), XMFLOAT4(0, 0, -1, 0), XMFLOAT2(0,1)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },
        
        { XMFLOAT3(1, -1, 1), XMFLOAT4(0, 0, -1, 0), XMFLOAT2(1,1)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },
        { XMFLOAT3(1, 1, 1), XMFLOAT4(0, 0, -1, 0), XMFLOAT2(1,0) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},
        { XMFLOAT3(-1, -1, 1), XMFLOAT4(0, 0, -1, 0), XMFLOAT2(0,1) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},
        
        //back face
        { XMFLOAT3(-1, -1, -1), XMFLOAT4(0, 0, 1, 0), XMFLOAT2(1,1) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(1, 1, -1), XMFLOAT4(0, 0, 1, 0), XMFLOAT2(0,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(-1, 1, -1), XMFLOAT4(0, 0, 1, 0), XMFLOAT2(1,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},

        { XMFLOAT3(-1, -1, -1), XMFLOAT4(0, 0, 1, 0), XMFLOAT2(1,1) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(1, -1, -1), XMFLOAT4(0, 0, 1, 0), XMFLOAT2(0,1) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(1, 1, -1), XMFLOAT4(0, 0,1, 0), XMFLOAT2(0,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        
        //left face
        { XMFLOAT3(-1, 1, 1), XMFLOAT4(1, 0, 0, 0), XMFLOAT2(1,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(-1, 1, -1), XMFLOAT4(1, 0, 0, 0), XMFLOAT2(0,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(-1, -1, -1), XMFLOAT4(1, 0, 0, 0), XMFLOAT2(0,1) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},

        { XMFLOAT3(-1, -1, -1), XMFLOAT4(1, 0, 0, 0), XMFLOAT2(0,1)/*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/ },
        { XMFLOAT3(-1, -1, 1), XMFLOAT4(1, 0, 0, 0), XMFLOAT2(1,1)/*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/ },
        { XMFLOAT3(-1, 1, 1), XMFLOAT4(1, 0, 0, 0), XMFLOAT2(1,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},

        //right face
        { XMFLOAT3(1, 1, 1), XMFLOAT4(-1, 0, 0, 0), XMFLOAT2(0,0)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },
        { XMFLOAT3(1, -1, -1), XMFLOAT4(-1, 0, 0, 0), XMFLOAT2(1,1)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },
        { XMFLOAT3(1, 1, -1), XMFLOAT4(-1, 0, 0, 0), XMFLOAT2(1,0) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},

        { XMFLOAT3(1, -1, -1), XMFLOAT4(-1, 0, 0, 0), XMFLOAT2(1,1) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},
        { XMFLOAT3(1, 1, 1), XMFLOAT4(-1, 0, 0, 0), XMFLOAT2(0,0) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},
        { XMFLOAT3(1, -1, 1), XMFLOAT4(-1, 0, 0, 0), XMFLOAT2(0,1) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},

        //top face
        { XMFLOAT3(-1, 1, -1), XMFLOAT4(0, -1, 0, 0), XMFLOAT2(0,0) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},
        { XMFLOAT3(-1, 1, 1), XMFLOAT4(0, -1, 0, 0), XMFLOAT2(0,1)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },
        { XMFLOAT3(1, 1, -1), XMFLOAT4(0, -1, 0, 0), XMFLOAT2(1,0) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},

        { XMFLOAT3(1, 1, -1), XMFLOAT4(0, -1, 0, 0), XMFLOAT2(1,0)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },
        { XMFLOAT3(-1, 1, 1), XMFLOAT4(0, -1, 0, 0), XMFLOAT2(0,1) /*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/},
        { XMFLOAT3(1, 1, 1), XMFLOAT4(0, -1, 0, 0), XMFLOAT2(1,1)/*XMFLOAT3(-1,0,0), XMFLOAT3(0,-1,0)*/ },

        //bottom face
        { XMFLOAT3(-1, -1, -1), XMFLOAT4(0, 1, 0, 0), XMFLOAT2(1,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(-1, -1, 1), XMFLOAT4(0, 1, 0, 0), XMFLOAT2(1,1) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(1, -1, -1), XMFLOAT4(0, 1, 0, 0), XMFLOAT2(0,0) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},

        { XMFLOAT3(1, -1, -1), XMFLOAT4(0, 1, 0, 0), XMFLOAT2(0,0)/*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/ },
        { XMFLOAT3(1, -1, 1), XMFLOAT4(0, 1, 0, 0), XMFLOAT2(0,1) /*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/},
        { XMFLOAT3(-1, -1, 1), XMFLOAT4(0, 1, 0, 0), XMFLOAT2(1,1)/*XMFLOAT3(1,0,0), XMFLOAT3(0,1,0)*/ },
    };

    D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertexNormal) * 36;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = cubeVertices;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pVertexBuffer);

    if (FAILED(hr))
        return hr;

    SimpleVertex pyramidVertices[] =
    {
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -2.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -2.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(0.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 5;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = pyramidVertices;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPyramidVertexBuffer);

    if (FAILED(hr))
        return hr;
    /*
    SimpleVertex groundPlaneVertices[] = 
    {
        { XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, 0.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.5f, 0.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 0.0f, -0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, 0.0f, -0.5f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, -0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.5f, 0.0f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 0.0f, -0.5f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, 0.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.5f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 0.0f, 0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, 0.0f, 0.5f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, 0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.5f, 0.0f, 0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 0.0f, 0.5f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.5f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) }
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 25;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = groundPlaneVertices;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pGroundPlaneVertexBuffer);

    if (FAILED(hr))
        return hr;
    */

	return S_OK;
}

HRESULT Application::InitIndexBuffer()
{
    HRESULT hr;

    // Create index buffer
    WORD indices[] =
    {
        /*
        //back face
        0,1,2,
        2,1,3,
        //left face
        0,2,6,
        4,0,6,
        //right face
        1,5,7,
        1,7,3,
        //front face
        7,5,4,
        6,7,4,
        //top face
        0,4,5,
        5,1,0,
        //bottom face
        7,6,2,
        7,2,3
        */
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35
    };

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(indices) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pIndexBuffer);

    if (FAILED(hr))
        return hr;	

    WORD pyramidIndices[] =
    {
        //base of pyramid
        0,1,2,
        0,2,3,
        1,0,4,
        4,2,1,
        3,4,0,
        3,2,4
    };

    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(pyramidIndices) * 18;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = pyramidIndices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPyramidIndexBuffer);

    if (FAILED(hr))
        return hr;
    /*
    WORD groundPlaneIndices[] =
    {
        //first row of Squares
        0,1,5,
        1,6,5,
        1,2,6,
        2,7,6,
        2,3,7,
        3,8,7,
        3,4,8,
        4,9,8,
        //second row of Squares
        5,6,10,
        6,11,10,
        6,7,11,
        7,12,11,
        7,8,12,
        8,13,12,
        8,9,13,
        9,14,13,
        //third row of Squares
        10,11,15,
        11,16,15,
        11,12,16,
        12,17,26,
        12,13,17,
        13,18,17,
        13,14,18,
        14,19,18,
        //fourth row of Squares
        15,16,20,
        16,21,20,
        16,17,21,
        17,22,21,
        17,18,22,
        18,23,22,
        18,19,23,
        19,24,23
    };

    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(groundPlaneIndices) * 96;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = groundPlaneIndices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pGroundPlaneIndexBuffer);

    if (FAILED(hr))
        return hr;
    */

	return S_OK;
}
#include "iostream"
HRESULT Application::InitPlaneIndexBuffer(int vWidth, int vHeight)
{
    HRESULT hr;
    D3D11_BUFFER_DESC bd;
    D3D11_SUBRESOURCE_DATA InitData;

    WORD* indices = new WORD[(vWidth - 1) * (vHeight - 1) * 6];

    UINT k = 0;
    for (UINT i = 0; i < vWidth - 1; ++i)
    {
        for (UINT j = 0; j < vHeight - 1; ++j)
        {
            indices[k + 0] = i * vHeight + j;  //0
            indices[k + 1] = i * vHeight + j + 1; //1
            indices[k + 2] = (i + 1) * vHeight + j;  //3
            indices[k + 3] = (i + 1) * vHeight + j;  //3
            indices[k + 4] = i * vHeight + j + 1;  //1
            indices[k + 5] = (i + 1) * vHeight + j + 1; //2
            k += 6;
        }
    }
    
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * ((vWidth-1) * (vHeight-1) * 6);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pGroundPlaneIndexBuffer);

    if (FAILED(hr))
        return hr;

    return S_OK;
}

HRESULT Application::InitPlaneVertexBuffer(UINT width, UINT depth, UINT Wverts, UINT Dverts)
{
    HRESULT hr;
    D3D11_BUFFER_DESC bd;
    D3D11_SUBRESOURCE_DATA InitData;

    float vCount = Wverts * Dverts;

    float hWidth = 0.5 * width;
    float hDepth = 0.5 * depth;

    float dx = width / (Wverts - 1);
    float dz = depth / (Dverts - 1);

    float du = 1.0f / (Wverts - 1);
    float dv = 1.0f / (Dverts - 1);


    SimpleVertexNormal* vertices = new SimpleVertexNormal[vCount];

    for (UINT i = 0; i < Wverts; ++i)
    {
        float z = hDepth - i * dz;
        for (UINT j = 0; j < Dverts; ++j)
        {
            float x = -hWidth + j * dx;
            vertices[i * Dverts + j].Pos = XMFLOAT3(x, 0, z);
            vertices[i * Dverts + j].normal = XMFLOAT4(0, -1, 0, 0);
            // Ignore for now, used for texturing.
            vertices[i * Dverts + j].TexC.x = j * du;
            vertices[i * Dverts + j].TexC.y = i * dv;

        }
    }

    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertexNormal) * vCount;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pGroundPlaneVertexBuffer);

    if (FAILED(hr))
        return hr;

    return S_OK;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    _hInst = hInstance;
    RECT rc = {0, 0, 640, 480};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindow(L"TutorialWindowClass", L"DX11 Framework", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                         nullptr);
    if (!_hWnd)
		return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);

    return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob != nullptr)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

        if (pErrorBlob) pErrorBlob->Release();

        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT Application::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _WindowWidth;
    sd.BufferDesc.Height = _WindowHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    D3D11_TEXTURE2D_DESC depthStencilDesc;

    depthStencilDesc.Width = _WindowWidth;
    depthStencilDesc.Height = _WindowHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;
 
    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;
    _pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);
    _pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);
    _pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_WindowWidth;
    vp.Height = (FLOAT)_WindowHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    _pd3dDevice->CreateSamplerState(&sampDesc, &_pSamplerLinear);

	InitShadersAndInputLayout();

	InitVertexBuffer();

	InitIndexBuffer();

    InitPlaneVertexBuffer(10, 10, 11, 11);

    InitPlaneIndexBuffer(11,11);

    // Set primitive topology
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &_pConstantBuffer);

    if (FAILED(hr))
        return hr;

    D3D11_RASTERIZER_DESC wfdesc;
    ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
    wfdesc.FillMode = D3D11_FILL_WIREFRAME;
    wfdesc.CullMode = D3D11_CULL_NONE;
    hr = _pd3dDevice->CreateRasterizerState(&wfdesc, &_wireFrame);
    _pImmediateContext->RSSetState(_wireFrame);

    D3D11_RASTERIZER_DESC soliddesc;
    ZeroMemory(&soliddesc, sizeof(D3D11_RASTERIZER_DESC));
    soliddesc.FillMode = D3D11_FILL_SOLID;
    soliddesc.CullMode = D3D11_CULL_NONE;
    hr = _pd3dDevice->CreateRasterizerState(&soliddesc, &_solidObj);
    _pImmediateContext->RSSetState(_solidObj);

    return S_OK;
}

void Application::Cleanup()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();
    if (_pConstantBuffer) _pConstantBuffer->Release();
    if (_pVertexBuffer) _pVertexBuffer->Release();
    if (_pIndexBuffer) _pIndexBuffer->Release();
    if (_pVertexLayout) _pVertexLayout->Release();
    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();
    if (_pRenderTargetView) _pRenderTargetView->Release();
    if (_pSwapChain) _pSwapChain->Release();
    if (_pImmediateContext) _pImmediateContext->Release();
    if (_pd3dDevice) _pd3dDevice->Release();
    if (_depthStencilView) _depthStencilView->Release();
    if (_depthStencilBuffer) _depthStencilBuffer->Release();
    if (_wireFrame) _wireFrame->Release();
}

void Application::Update()
{
    // Update our time
    static float t = 0.0f;

    if (_driverType == D3D_DRIVER_TYPE_REFERENCE)
    {
        t += (float) XM_PI * 0.0125f;
    }
    else
    {
        static DWORD dwTimeStart = 0;
        DWORD dwTimeCur = GetTickCount();

        if (dwTimeStart == 0)
            dwTimeStart = dwTimeCur;

        t = (dwTimeCur - dwTimeStart) / 10000.0f;
    }

    //
    // Animate
    //
    XMMATRIX sun = XMMatrixIdentity();
    sun = XMMatrixMultiply(sun, XMMatrixScaling(3,3,3) * XMMatrixTranslation(0, 0, 0) * XMMatrixRotationRollPitchYaw(t*10, t*10, t*10));
    XMStoreFloat4x4(&_worldMatrices[0], sun);

    XMMATRIX mars = XMMatrixIdentity();
    mars = XMMatrixMultiply(mars, XMMatrixRotationRollPitchYaw(t * 4, 0, 0) * XMMatrixScaling(.6, .6, .6) * 
    XMMatrixTranslation(6, 0, 0) * XMMatrixRotationRollPitchYaw(0, t*5, 0));
    XMStoreFloat4x4(&_worldMatrices[1], mars);

    XMMATRIX earth = XMMatrixIdentity();
    earth = XMMatrixMultiply(earth, XMMatrixRotationRollPitchYaw(0, 0, t * 6) * XMMatrixScaling(.8, .8, .8) * 
    XMMatrixTranslation(9, 0, 0) * XMMatrixRotationRollPitchYaw(0, t * 3.5, 0));
    XMStoreFloat4x4(&_worldMatrices[2], earth);

    XMMATRIX earthMoon = XMMatrixIdentity();
    //translation are done backwards, so you would read the first to be the last
    //scale the moon translate the moon to its position relative to the earth, allow the moon to rotate around the earth, 
    //translate the mmon to the position of the eath and allow the same rotation (so it follows the earths rotatrion around the sun)
    earthMoon = XMMatrixMultiply(earthMoon, XMMatrixScaling(.125, .125, .125) * XMMatrixTranslation(3,0,0) * 
    XMMatrixRotationRollPitchYaw(0, t*2, t * 3) * XMMatrixTranslation(9, 0, 0) * XMMatrixRotationRollPitchYaw(0, t * 3.5, 0));
    XMStoreFloat4x4(&_worldMatrices[3], earthMoon);

    XMMATRIX marsMoon = XMMatrixIdentity();
    marsMoon = XMMatrixMultiply(marsMoon, XMMatrixScaling(.1, .1, .1) * XMMatrixTranslation(3, 0, 0) * 
    XMMatrixRotationRollPitchYaw(0, t * 2, t * 4) * XMMatrixTranslation(6, 0, 0) * XMMatrixRotationRollPitchYaw(0, t * 5, 0));
    XMStoreFloat4x4(&_worldMatrices[4], marsMoon);

    XMMATRIX floor = XMMatrixIdentity();
    floor = XMMatrixMultiply(floor, XMMatrixScaling(3, 3, 3) * XMMatrixTranslation(0, -8, 0));
    XMStoreFloat4x4(&_groundPlaneMatrix, floor);
}

void Application::Draw()
{
    //
    // Clear the back buffer
    //
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red,green,blue,alpha
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
    _pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Set vertex buffer
    UINT stride = sizeof(SimpleVertexNormal);
    UINT offset = 0;
    for (int i = 0; i < 5; i++)
    {
        XMMATRIX world = XMLoadFloat4x4(&_worldMatrices[i]);
        XMMATRIX view = XMLoadFloat4x4(&_view);
        XMMATRIX projection = XMLoadFloat4x4(&_projection);
        //
        // Update variables
        //
        ConstantBuffer cb;
        cb.mWorld = XMMatrixTranspose(world);
        cb.mView = XMMatrixTranspose(view);
        cb.mProjection = XMMatrixTranspose(projection);
        
        cb.LightVecW = { 0,3,25 };
        cb.EyePosW = { 0, 0, 25 };

        //ambient is the actual colour of the object
        cb.AmbientMtrl = { 1,.2,.2,.2 };
        cb.AmbientLight = { 1,.2,.2,.2 };

        //light that reflect off of other objects in the world
        cb.DiffuseMtrl = { 1,.5,.4,.1 };
        cb.DiffuseLight = { 0,0,0,1 };

        cb.SpecularMtrl = { 1,.5,.5,.5 };
        cb.SpecularLight = { 1,.8,.8,.8 };
        cb.SpecularPower = 10;

        _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);
        // Renders a triangle
        _pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);
        _pImmediateContext->PSSetShaderResources(0, 1, &_pTextureRV);
        _pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
        _pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
        _pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
        _pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
        if (i >= 2)
        {
            // Set vertex buffer
            _pImmediateContext->IASetVertexBuffers(0, 1, &_pPyramidVertexBuffer, &stride, &offset);
            // Set index buffer
            _pImmediateContext->IASetIndexBuffer(_pPyramidIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
            _pImmediateContext->RSSetState(_solidObj);
            _pImmediateContext->DrawIndexed(18, 0, 0);
        }
        else {
            // Set vertex buffer
            _pImmediateContext->IASetVertexBuffers(0, 1, &_pVertexBuffer, &stride, &offset);
            // Set index buffer
            _pImmediateContext->IASetIndexBuffer(_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
            _pImmediateContext->RSSetState(_solidObj);
            _pImmediateContext->DrawIndexed(36, 0, 0);
        }
    }
    XMMATRIX world = XMLoadFloat4x4(&_groundPlaneMatrix);
    XMMATRIX view = XMLoadFloat4x4(&_view);
    XMMATRIX projection = XMLoadFloat4x4(&_projection);
    //
    // Update variables
    //
    ConstantBuffer cb1;
    cb1.mWorld = XMMatrixTranspose(world);
    cb1.mView = XMMatrixTranspose(view);
    cb1.mProjection = XMMatrixTranspose(projection);

    cb1.LightVecW = { 0,0,0 };
    cb1.EyePosW = { 0, 0, 25 };

    //ambient is the actual colour of the object
    cb1.AmbientMtrl = { 1,.2,.2,.2 };
    cb1.AmbientLight = { 1,.2,.2,.2 };

    //light that reflect off of other objects in the world
    cb1.DiffuseMtrl = { 1,.5,.4,.1 };
    cb1.DiffuseLight = { 0,0,0,1 };

    cb1.SpecularMtrl = { 1,.5,.5,.5 };
    cb1.SpecularLight = { 1,.8,.8,.8 };
    cb1.SpecularPower = 10;

    _pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb1, 0, 0);

    //
    // Renders a triangle
    //

    _pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
    _pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
    _pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);
    // Set vertex buffer
    _pImmediateContext->IASetVertexBuffers(0, 1, &_pGroundPlaneVertexBuffer, &stride, &offset);
    // Set index buffer
    _pImmediateContext->IASetIndexBuffer(_pGroundPlaneIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    _pImmediateContext->RSSetState(_solidObj);
    _pImmediateContext->DrawIndexed(600, 0, 0);
    //
    // Present our back buffer to our front buffer
    //
    _pSwapChain->Present(0, 0);
}