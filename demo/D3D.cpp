#include "stdafx.h"
#include "D3D.h"
#include "Effects\\ShaderCommon.hlsl"

#include <sys/types.h>
#include <sys/stat.h>

ID3D11BlendState* BlendStates::_BsDefault = NULL;
ID3D11BlendState* BlendStates::_BsAdditive = NULL;
ID3D11BlendState* BlendStates::_BsBlendOnBack = NULL;
ID3D11BlendState* BlendStates::_BsInvert = NULL;

bool BlendStates::CreateDevice(ID3D11Device* Device)
{
	// -----------------------------------------
	D3D11_BLEND_DESC bsDefault;
	ZeroMemory(&bsDefault, sizeof(D3D11_BLEND_DESC));
	bsDefault.AlphaToCoverageEnable = false;
	bsDefault.IndependentBlendEnable = false;
	for (int i = 0; i < 8; ++i)
	{
		bsDefault.RenderTarget[i].BlendEnable = false;
		bsDefault.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		bsDefault.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bsDefault.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
		bsDefault.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		bsDefault.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		bsDefault.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		bsDefault.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
	}
	if (FAILED( Device->CreateBlendState(&bsDefault, &_BsDefault))) return false;
	
	// -----------------------------------------
	D3D11_BLEND_DESC bsAdditive;
	ZeroMemory(&bsAdditive, sizeof(D3D11_BLEND_DESC));
	bsAdditive.AlphaToCoverageEnable = false;
	bsAdditive.IndependentBlendEnable = false;
	for (int i = 0; i < 8; ++i)
	{
		bsAdditive.RenderTarget[i].BlendEnable = true;
		bsAdditive.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		bsAdditive.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bsAdditive.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
		bsAdditive.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
		bsAdditive.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		bsAdditive.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		bsAdditive.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
	}
	if (FAILED( Device->CreateBlendState(&bsAdditive, &_BsAdditive))) return false;

	// -----------------------------------------
	{
		D3D11_BLEND_DESC bsBlend;
		ZeroMemory(&bsBlend, sizeof(D3D11_BLEND_DESC));
		bsBlend.AlphaToCoverageEnable = false;
		bsBlend.IndependentBlendEnable = false;
		for (int i = 0; i < 8; ++i)
		{
			bsBlend.RenderTarget[i].BlendEnable = true;
			bsBlend.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
			bsBlend.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bsBlend.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bsBlend.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			bsBlend.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			bsBlend.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
			bsBlend.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		}
		if (FAILED( Device->CreateBlendState(&bsBlend, &_BsBlendOnBack))) return false;
	}

	// -----------------------------------------
	{
		D3D11_BLEND_DESC bsBlend;
		ZeroMemory(&bsBlend, sizeof(D3D11_BLEND_DESC));
		bsBlend.AlphaToCoverageEnable = false;
		bsBlend.IndependentBlendEnable = false;
		for (int i = 0; i < 8; ++i)
		{
			bsBlend.RenderTarget[i].BlendEnable = true;
			bsBlend.RenderTarget[i].BlendOp = D3D11_BLEND_OP_SUBTRACT;
			bsBlend.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bsBlend.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
			bsBlend.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
			bsBlend.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			bsBlend.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
			bsBlend.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ZERO;
		}
		if (FAILED(Device->CreateBlendState(&bsBlend, &_BsInvert))) return false;
	}
	
	return true;
}

// Release resources that depend on the device.
void BlendStates::ReleaseDevice()
{
	SAFE_RELEASE(_BsDefault);
	SAFE_RELEASE(_BsAdditive);
	SAFE_RELEASE(_BsBlendOnBack);
	SAFE_RELEASE(_BsInvert);
}


ID3D11DepthStencilState* DepthStencilStates::_DsTestWriteOn = NULL;
ID3D11DepthStencilState* DepthStencilStates::_DsTestWriteOff = NULL;

// Create resources that depend on the device.
bool DepthStencilStates::CreateDevice(ID3D11Device* Device)
{
	// -----------------------------------------
	D3D11_DEPTH_STENCIL_DESC dsTestWriteOn;
	ZeroMemory(&dsTestWriteOn, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsTestWriteOn.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsTestWriteOn.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsTestWriteOn.DepthEnable = true;
	dsTestWriteOn.StencilEnable = false;
	if (FAILED(Device->CreateDepthStencilState(&dsTestWriteOn, &_DsTestWriteOn))) return false;
	
	// -----------------------------------------
	D3D11_DEPTH_STENCIL_DESC dsTestWriteOff;
	ZeroMemory(&dsTestWriteOff, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsTestWriteOff.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsTestWriteOff.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsTestWriteOff.DepthEnable = false;
	dsTestWriteOff.StencilEnable = false;
	if (FAILED(Device->CreateDepthStencilState(&dsTestWriteOff, &_DsTestWriteOff))) return false;
	
	return true;
}

// Release resources that depend on the device.
void DepthStencilStates::ReleaseDevice()
{
	SAFE_RELEASE(_DsTestWriteOn);
	SAFE_RELEASE(_DsTestWriteOff);
}


ID3D11RasterizerState* RasterizerStates::_RsCullBack = NULL;
ID3D11RasterizerState* RasterizerStates::_RsCullFront = NULL;
ID3D11RasterizerState* RasterizerStates::_RsCullNone = NULL;
ID3D11RasterizerState* RasterizerStates::_RsWireCullNone = NULL;

// Create resources that depend on the device.
bool RasterizerStates::CreateDevice(ID3D11Device* Device)
{
    // -----------------------------------------
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.DepthBias = 0;
    rsDesc.DepthBiasClamp = 0;
	rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.AntialiasedLineEnable = false;
    rsDesc.DepthClipEnable = true;
    rsDesc.FrontCounterClockwise = true;
    rsDesc.MultisampleEnable = true;
    rsDesc.ScissorEnable = false;
    rsDesc.SlopeScaledDepthBias = 0;
	if (FAILED(Device->CreateRasterizerState(&rsDesc, &_RsCullBack))) return false;
	
	// -----------------------------------------
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));
    rsDesc.CullMode = D3D11_CULL_FRONT;
    rsDesc.DepthBias = 0;
    rsDesc.DepthBiasClamp = 0;
	rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.AntialiasedLineEnable = false;
    rsDesc.DepthClipEnable = true;
    rsDesc.FrontCounterClockwise = true;
    rsDesc.MultisampleEnable = true;
    rsDesc.ScissorEnable = false;
    rsDesc.SlopeScaledDepthBias = 0;
	if (FAILED(Device->CreateRasterizerState(&rsDesc, &_RsCullFront))) return false;

    // -----------------------------------------            
	rsDesc.CullMode = D3D11_CULL_NONE;
	if (FAILED(Device->CreateRasterizerState(&rsDesc, &_RsCullNone))) return false;

	// -----------------------------------------            
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	if (FAILED(Device->CreateRasterizerState(&rsDesc, &_RsWireCullNone))) return false;
	
    return true;
}

// Release resources that depend on the device.
void RasterizerStates::ReleaseDevice()
{
	SAFE_RELEASE(_RsCullBack);
	SAFE_RELEASE(_RsCullFront);
	SAFE_RELEASE(_RsCullNone);
	SAFE_RELEASE(_RsWireCullNone);
}

ID3D11SamplerState* SamplerStates::_MinMagLinearMipPoint_Border0 = NULL;

// Gets the state that enabled backface culling.
ID3D11SamplerState* SamplerStates::MinMagLinearMipPointBorder0() { return _MinMagLinearMipPoint_Border0; }

// Create resources that depend on the device.
bool SamplerStates::CreateDevice(ID3D11Device* Device)
{
    // -----------------------------------------
	D3D11_SAMPLER_DESC sDesc;
	ZeroMemory(&sDesc, sizeof(D3D11_SAMPLER_DESC));
	sDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sDesc.BorderColor[0] = sDesc.BorderColor[1] = sDesc.BorderColor[2] = sDesc.BorderColor[3] = 0;
	sDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	
	if (FAILED(Device->CreateSamplerState(&sDesc, &_MinMagLinearMipPoint_Border0))) return false;
    // -----------------------------------------            
	
    return true;
}

// Release resources that depend on the device.
void SamplerStates::ReleaseDevice()
{
	SAFE_RELEASE(_MinMagLinearMipPoint_Border0);
}

// ================================================================================
D3D::D3D(HWND WindowHandle) : 
_WindowHandle(WindowHandle),
_Device(NULL),
_ImmediateContext(NULL),
_Adapter(NULL),
_Factory(NULL),
_SwapChain(NULL),
_RtvBackbuffer(NULL),
_TexBackbuffer(NULL),
_TexBackbufferSingleSampled(NULL),
_DsvBackbuffer(NULL),
_SrvDepthbuffer(NULL),
_FullViewport(D3D11_VIEWPORT()),
_EventQuery(NULL)
{
    _BackBufferSurfaceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	RECT windowRect;
	GetClientRect(_WindowHandle, &windowRect);

	_BackBufferSurfaceDesc.Width = windowRect.right - windowRect.left;
	_BackBufferSurfaceDesc.Height = windowRect.bottom - windowRect.top;

	// 4xMSAA: 4,4  |  8xCSAA:4,8  |  16xCSAA:4,16
#ifdef MSAA_SAMPLES
    _BackBufferSurfaceDesc.SampleDesc.Count = MSAA_SAMPLES;
    _BackBufferSurfaceDesc.SampleDesc.Quality = 0; // MSAA_SAMPLES == 1 ? 0 : 16;	
#else
	_BackBufferSurfaceDesc.SampleDesc.Count = 1;
    _BackBufferSurfaceDesc.SampleDesc.Quality = 0;
#endif
}

// ================================================================================
D3D::~D3D()
{		
    ReleaseSwapChain();
    ReleaseDevice();
}

// ================================================================================
bool D3D::CreateDevice()
{
    if(FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory) ,(void**)&_Factory)))		
        _Factory = NULL;

    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_UNKNOWN,        
    };
    UINT numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	unsigned int numFeatureLevels = 1;
	
	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		_DriverType = driverTypes[driverTypeIndex];
		
		for ( UINT i = 0;
			  _Factory->EnumAdapters(i, &_Adapter) != DXGI_ERROR_NOT_FOUND;
			  ++i )
		{
			DXGI_ADAPTER_DESC desc;
			_Adapter->GetDesc(&desc);

			std::wstring adapterDesc(desc.Description);
			wprintf(L"Using GPU: %ls\n", adapterDesc.c_str());

			hr = D3D11CreateDevice(_Adapter, _DriverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION,  &_Device, &_FeatureLevel, &_ImmediateContext);
			if (SUCCEEDED(hr))
				break;
		}

        if( SUCCEEDED( hr ) )
            break;
    }

	// silcence selected messages
	ID3D11Debug *d3dDebug = nullptr;
	if (SUCCEEDED(_Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug)))
	{
		ID3D11InfoQueue *d3dInfoQueue = nullptr;
		if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
		{
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_DEVICE_DRAW_RESOURCE_FORMAT_SAMPLE_UNSUPPORTED,
				// Add more message IDs here as needed
			};
			D3D11_INFO_QUEUE_FILTER filter;
			memset(&filter, 0, sizeof(filter));
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
			d3dInfoQueue->Release();
		}
		d3dDebug->Release();
	}

	D3D11_QUERY_DESC qd;
	qd.Query = D3D11_QUERY_EVENT;
	qd.MiscFlags = 0;
	_Device->CreateQuery( &qd, &_EventQuery );

	if (!BlendStates::CreateDevice(_Device)) return false;
	if (!DepthStencilStates::CreateDevice(_Device)) return false;
	if (!RasterizerStates::CreateDevice(_Device)) return false;
	if (!SamplerStates::CreateDevice(_Device)) return false;

    return _D3DCreateDevice_Callback(_Device, &_BackBufferSurfaceDesc);
}

// ================================================================================
bool D3D::CreateSwapChain()
{
    HRESULT hr = S_OK;

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _BackBufferSurfaceDesc.Width;
    sd.BufferDesc.Height = _BackBufferSurfaceDesc.Height;
    sd.BufferDesc.Format = _BackBufferSurfaceDesc.Format;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _WindowHandle;
    sd.SampleDesc.Count = _BackBufferSurfaceDesc.SampleDesc.Count;
    sd.SampleDesc.Quality = _BackBufferSurfaceDesc.SampleDesc.Quality;
    sd.Windowed = TRUE;

    if (!_Device)
    {
        ReleaseDevice();
        CreateDevice();		
    }
    
    hr = _Factory->CreateSwapChain(_Device, &sd, &_SwapChain);
	if (FAILED(hr)) return false;
    
    // Create a render target view	    
    hr = _SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&_TexBackbuffer );
    if( FAILED( hr ) )
        return false;	

    hr = _Device->CreateRenderTargetView( _TexBackbuffer, NULL, &_RtvBackbuffer );
    if (FAILED(hr)) return false;
    
    // Create depth stencil view	
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(D3D11_TEXTURE2D_DESC));
    descDepth.Width = _BackBufferSurfaceDesc.Width;
    descDepth.Height = _BackBufferSurfaceDesc.Height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
    descDepth.SampleDesc.Count = _BackBufferSurfaceDesc.SampleDesc.Count;
    descDepth.SampleDesc.Quality = _BackBufferSurfaceDesc.SampleDesc.Quality;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    ID3D11Texture2D* pDSTexture;
    hr = _Device->CreateTexture2D( &descDepth, NULL, &pDSTexture );
    if (FAILED(hr)) return false;

	ID3D11Texture2D* pDSTextureSingleSampled = NULL;
#ifdef MSAA_SAMPLES
	descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
	hr = _Device->CreateTexture2D( &descDepth, NULL, &pDSTextureSingleSampled );

	D3D11_TEXTURE2D_DESC backDesc;
	ZeroMemory(&backDesc, sizeof(D3D11_TEXTURE2D_DESC));
	backDesc.ArraySize = 1;
	backDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	backDesc.Width = _BackBufferSurfaceDesc.Width;
	backDesc.Height = _BackBufferSurfaceDesc.Height;
	backDesc.MipLevels = 1;
	backDesc.SampleDesc.Count = 1;
	hr = _Device->CreateTexture2D( &backDesc, NULL, &_TexBackbufferSingleSampled );
#endif
	

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    if (_BackBufferSurfaceDesc.SampleDesc.Count == 1 && _BackBufferSurfaceDesc.SampleDesc.Quality == 0)
        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;	
    else descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    descDSV.Texture2D.MipSlice = 0;
    
    // Create the depth stencil view	
    hr = _Device->CreateDepthStencilView( pDSTexture, // Depth stencil texture
                                             &descDSV, // Depth stencil desc
                                             &_DsvBackbuffer );  // [out] Depth stencil view
    if (FAILED(hr)) return false;

	
	D3D11_SHADER_RESOURCE_VIEW_DESC resDesc;
    ZeroMemory(&resDesc, sizeof(resDesc));
    resDesc.Format = DXGI_FORMAT_R32_FLOAT;
	if (_BackBufferSurfaceDesc.SampleDesc.Count == 1 && _BackBufferSurfaceDesc.SampleDesc.Quality == 0)
        resDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;	
    else resDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    resDesc.Texture2D.MostDetailedMip = 0;
    resDesc.Texture2D.MipLevels = 1;
	hr = _Device->CreateShaderResourceView( pDSTexture, &resDesc, &_SrvDepthbuffer);
	if (FAILED(hr)) return false;

    SAFE_RELEASE(pDSTexture);
	SAFE_RELEASE(pDSTextureSingleSampled);

    _ImmediateContext->OMSetRenderTargets( 1, &_RtvBackbuffer, _DsvBackbuffer );
    
    // Setup the viewport    
    _FullViewport.Width = (FLOAT) _BackBufferSurfaceDesc.Width;
    _FullViewport.Height = (FLOAT) _BackBufferSurfaceDesc.Height;
    _FullViewport.MinDepth = 0.0f;
    _FullViewport.MaxDepth = 1.0f;
    _FullViewport.TopLeftX = 0;
    _FullViewport.TopLeftY = 0;
    _ImmediateContext->RSSetViewports( 1, &_FullViewport );

    return _D3DCreateSwapChain_Callback(_Device, _SwapChain, &_BackBufferSurfaceDesc);
}

// ================================================================================
void D3D::ReleaseDevice()
{
	SAFE_RELEASE(_EventQuery);
	if (_ImmediateContext) _ImmediateContext->ClearState();
	BlendStates::ReleaseDevice();
	DepthStencilStates::ReleaseDevice();
	RasterizerStates::ReleaseDevice();
	SamplerStates::ReleaseDevice();
    _D3DReleaseDevice_Callback();
	SAFE_RELEASE(_ImmediateContext);

	
	// print a more detailed view of the currently alive d3d objects
#if _DEBUG
	ID3D11Debug* debug;
	_Device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debug));
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	SAFE_RELEASE(debug);
#endif 
	
#ifndef _DEBUG
    SAFE_RELEASE(_Device);		
#endif
    SAFE_RELEASE(_Adapter);
    SAFE_RELEASE(_Factory);
}

// ================================================================================
void D3D::ReleaseSwapChain()
{
    _D3DReleaseSwapChain_Callback();
    SAFE_RELEASE(_SwapChain);
    SAFE_RELEASE(_RtvBackbuffer);
    SAFE_RELEASE(_DsvBackbuffer);
	SAFE_RELEASE(_SrvDepthbuffer);
	SAFE_RELEASE(_TexBackbuffer);
	SAFE_RELEASE(_TexBackbufferSingleSampled);
}

void D3D::WaitForGPU(ID3D11DeviceContext* immediateContext)
{
	immediateContext->Flush();
	immediateContext->End(_EventQuery);
	while( S_OK != immediateContext->GetData(_EventQuery,NULL,0,0) );
}

// ================================================================================
bool D3D::Init()
{
    if (!CreateDevice()) return false;
    if (!CreateSwapChain()) return false;
    return true;
}

// ================================================================================
void D3D::ExportBackbufferToFile()
{
	std::string path = "export";
	int i=0;
	bool canWriteHere = false;
	while (!canWriteHere)
	{
		std::stringstream ss;
		if ( i < 10 )
			ss << path << "_000" << i << ".bmp";
		else if ( i < 100 )
			ss << path << "_00" << i << ".bmp";
		else if ( i < 1000 )
			ss << path << "_0" << i << ".bmp";
		else
			ss << path << "_" << i << ".bmp";
		struct _stat stat;
		if (_stat(ss.str().c_str(), &stat) < 0)
		{
			canWriteHere = true;
			path = ss.str();
		}
		else ++i;
	}
#ifdef MSAA_SAMPLES
	_ImmediateContext->ResolveSubresource(_TexBackbufferSingleSampled, 0, _TexBackbuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	HRESULT hr = D3DX11SaveTextureToFile(_ImmediateContext, _TexBackbufferSingleSampled, D3DX11_IFF_BMP, path.c_str());
#else
	HRESULT hr = D3DX11SaveTextureToFile(_ImmediateContext, _TexBackbuffer, D3DX11_IFF_BMP, path.c_str());
#endif
}

// ================================================================================
void D3D::ExportBackbufferToFile(const char* path)
{
#ifdef MSAA_SAMPLES
	_ImmediateContext->ResolveSubresource(_TexBackbufferSingleSampled, 0, _TexBackbuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	HRESULT hr = D3DX11SaveTextureToFile(_ImmediateContext, _TexBackbufferSingleSampled, D3DX11_IFF_BMP, path);
#else
	HRESULT hr = D3DX11SaveTextureToFile(_ImmediateContext, _TexBackbuffer, D3DX11_IFF_BMP, path);
#endif
}

// ================================================================================
bool D3D::LoadEffectFromFile(const std::string& Filename, ID3D11Device* Device, ID3DX11Effect** Effect)
{
	char currentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDir);

	std::string path = std::string(currentDir) + "\\" + Filename;
	// Check if file exists
	{
		struct _stat buffer;
		int iRetTemp = 0;
		memset((void*)&buffer, 0, sizeof(buffer));
		iRetTemp = _stat(path.c_str(), &buffer);
		if (iRetTemp == 0)
		{
			if (! (buffer.st_mode & _S_IFMT))
			return false;
		}
		else
		{
			printf("Shader %s not found!\n", Filename.c_str());
			return false;
		}
	}
	
	// open file
	std::fstream file;
	file.open(path.c_str(), std::ios::binary | std::ios::in);
	
	// get length of file
	file.seekg (0, std::ios::end);
	UINT length = (UINT)file.tellg();
	file.seekg (0, std::ios::beg);	

	char* data = new char[length];
	file.read(data, length);
	
	HRESULT hr = D3DX11CreateEffectFromMemory(data, length, 0, Device, Effect);
	if (hr != S_OK)
	{
		SAFE_DELETE_ARRAY(data);
		file.close();
		return false;
	}
	SAFE_DELETE_ARRAY(data);
	file.close();
	return true;
}


bool D3D::LoadComputeShaderFromFile(const std::string& Filename, ID3D11Device* Device, ID3D11ComputeShader** outShader)
{
	char currentDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, currentDir);

	std::string path = std::string(currentDir) + "\\" + Filename;
	// Check if file exists
	{
		struct _stat buffer;
		int iRetTemp = 0;
		memset((void*)&buffer, 0, sizeof(buffer));
		iRetTemp = _stat(path.c_str(), &buffer);
		if (iRetTemp == 0)
		{
			if (! (buffer.st_mode & _S_IFMT))
			return false;
		}
		else return false;
	}
	
	// open file
	std::fstream file;
	file.open(path.c_str(), std::ios::binary | std::ios::in);
	
	// get length of file
	file.seekg (0, std::ios::end);
	UINT length = (UINT)file.tellg();
	file.seekg (0, std::ios::beg);	

	char* data = new char[length];
	file.read(data, length);

	HRESULT hr = Device->CreateComputeShader(data, length, NULL, outShader);
	if (hr != S_OK)
	{
		SAFE_DELETE(data);
		file.close();
		return false;
	}
	SAFE_DELETE(data);
	file.close();
	return true;
}

RenderTarget2D::RenderTarget2D(const std::string& DebugName, const DXGI_FORMAT& Format, bool HasUav) :
	_DebugName(DebugName),
	_Format(Format),
	_Tex(NULL),
	_TexDynamic(NULL),
	_TexStaging(NULL),
	_Rtv(NULL),
	_Srv(NULL),
	_Uav(NULL),
	_HasUav(HasUav),
	_Resolution(0, 0)
{

}

RenderTarget2D::~RenderTarget2D()
{
	D3DReleaseSwapChain();
}

bool RenderTarget2D::D3DCreateSwapChain(ID3D11Device* Device, IDXGISwapChain* SwapChain, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc)
{
	int numFloats = 1;
	if (_Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
		numFloats = 4;

	_Resolution = Vec2i((int)BackBufferSurfaceDesc->Width, (int)BackBufferSurfaceDesc->Height);
	_Data.resize(_Resolution.x() * _Resolution.y() * numFloats);

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = _Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	if (_HasUav)
		texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	texDesc.Width = BackBufferSurfaceDesc->Width;
	texDesc.Height = BackBufferSurfaceDesc->Height;

	D3D11_SUBRESOURCE_DATA init;
	ZeroMemory(&init, sizeof(D3D11_SUBRESOURCE_DATA));
	init.pSysMem = &_Data[0];
	init.SysMemPitch = sizeof(float) * _Resolution.x() * numFloats;


	HRESULT hr = Device->CreateTexture2D(&texDesc, &init, &_Tex);
	if (FAILED(hr))
		return false;

	hr = Device->CreateRenderTargetView(_Tex, NULL, &_Rtv);
	if (FAILED(hr))
		return false;

	hr = Device->CreateShaderResourceView(_Tex, NULL, &_Srv);
	if (FAILED(hr))
		return false;

	if (_HasUav)
	{
		hr = Device->CreateUnorderedAccessView(_Tex, NULL, &_Uav);
		if (FAILED(hr))
			return false;
	}

	texDesc.Usage = D3D11_USAGE_DYNAMIC;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	hr = Device->CreateTexture2D(&texDesc, &init, &_TexDynamic);
	if (FAILED(hr))
	{
		return false;
	}

	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.BindFlags = 0;
	hr = Device->CreateTexture2D(&texDesc, &init, &_TexStaging);
	if (FAILED(hr))
	{
		return false;
	}

	return hr == S_OK;
}

void RenderTarget2D::D3DReleaseSwapChain()
{
	SAFE_RELEASE(_Tex);
	SAFE_RELEASE(_TexDynamic);
	SAFE_RELEASE(_TexStaging);
	SAFE_RELEASE(_Rtv);
	SAFE_RELEASE(_Srv);
	SAFE_RELEASE(_Uav);
}

void RenderTarget2D::CopyToCPU(ID3D11DeviceContext* ImmediateContext)
{
	ImmediateContext->CopyResource(_TexStaging, _Tex);
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	if (SUCCEEDED(ImmediateContext->Map(_TexStaging, 0, D3D11_MAP_READ, 0, &mappedResource)))
	{
		BYTE* mappedData = reinterpret_cast<BYTE*>(mappedResource.pData); // destination
		BYTE* buffer = (BYTE*)_Data.data();			// source
		int rowspan = _Resolution.x() * sizeof(float);	// row-pitch in source
		for (UINT i = 0; i < (UINT)_Resolution.y(); ++i)		// copy in the data line by line
		{
			memcpy(buffer, mappedData, rowspan);
			mappedData += mappedResource.RowPitch;	// row-pitch in destination
			buffer += rowspan;
		}
		ImmediateContext->Unmap(_TexStaging, 0);
	}
}

void RenderTarget2D::CopyToGPU(ID3D11DeviceContext* ImmediateContext)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
	if (SUCCEEDED(ImmediateContext->Map(_TexDynamic, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
	{
		BYTE* mappedData = reinterpret_cast<BYTE*>(mappedResource.pData); // destination
		BYTE* buffer = (BYTE*)_Data.data();			// source
		int rowspan = _Resolution.x() * sizeof(float);	// row-pitch in source
		for (UINT i = 0; i < (UINT)_Resolution.y(); ++i)		// copy in the data line by line
		{
			memcpy(mappedData, buffer, rowspan);
			mappedData += mappedResource.RowPitch;	// row-pitch in destination
			buffer += rowspan;
		}
		ImmediateContext->Unmap(_TexDynamic, 0);
	}
	ImmediateContext->CopyResource(_Tex, _TexDynamic);
}