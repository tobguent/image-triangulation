#pragma once

#include <DXGI.h>
#include <D3D11.h>

// Some blending states.
class BlendStates
{
	private:
		
		// No blending.
		static ID3D11BlendState* _BsDefault;

		// Additive blending
		static ID3D11BlendState* _BsAdditive;

		// Blends: scr*1 + dst * (1-src.alpha)
		static ID3D11BlendState* _BsBlendOnBack;

		// Inverts the background
		static ID3D11BlendState* _BsInvert;

	public:

		// Gets the default blend state. (Blending disabled)
		static ID3D11BlendState* Default() { return _BsDefault; }

		// Gets an additive blend state.
		static ID3D11BlendState* Additive() { return _BsAdditive; }

		// Gets a blend state that blends: scr*1 + dst * (1-src.alpha)
		static ID3D11BlendState* BlendOnBack() { return _BsBlendOnBack; }

		// Gets a blend state that inverts the background
		static ID3D11BlendState* Invert() { return _BsInvert; }


		// Create resources that depend on the device.
		static bool CreateDevice(ID3D11Device* Device);

		// Release resources that depend on the device.
		static void ReleaseDevice();
};

// Some depth stencil states.
class DepthStencilStates
{
	private:

		// Depth testing and writing enabled.
		static ID3D11DepthStencilState* _DsTestWriteOn;

		// Depth testing and writing disabled.
		static ID3D11DepthStencilState* _DsTestWriteOff;

	public:

		// Depth testing and writing enabled.
		static ID3D11DepthStencilState* DsTestWriteOn() { return _DsTestWriteOn; }

		// Depth testing and writing disabled.
		static ID3D11DepthStencilState* DsTestWriteOff() { return _DsTestWriteOff; }

		// Create resources that depend on the device.
		static bool CreateDevice(ID3D11Device* Device);

		// Release resources that depend on the device.
		static void ReleaseDevice();
};


// Contains the most common blending states.
class RasterizerStates
{
	private:

		// Backface culling.
		static ID3D11RasterizerState* _RsCullBack;

		// Frontface culling.
		static ID3D11RasterizerState* _RsCullFront;

		// No face culling.
		static ID3D11RasterizerState* _RsCullNone;

		// No face culling.
		static ID3D11RasterizerState* _RsWireCullNone;

	public:

		// Gets the state that enabled backface culling.
		static ID3D11RasterizerState* CullBack() { return _RsCullBack; }

		// Gets the state that enabled frontface culling.
		static ID3D11RasterizerState* CullFront() { return _RsCullFront; }

		// Gets the state that disables face culling.
		static ID3D11RasterizerState* CullNone() { return _RsCullNone; }

		// Gets the state that disables face culling.
		static ID3D11RasterizerState* WireCullNone() { return _RsWireCullNone; }

		// Create resources that depend on the device.
		static bool CreateDevice(ID3D11Device* Device);
		
		// Release resources that depend on the device.
		static void ReleaseDevice();
};

// Contains the most common blending states.
class SamplerStates
{
	private:
		
		// Linear filtering with zero border.
		static ID3D11SamplerState* _MinMagLinearMipPoint_Border0;

	public:

		// Gets the linear filtering with zero border.
		static ID3D11SamplerState* MinMagLinearMipPointBorder0();

		// Create resources that depend on the device.
		static bool CreateDevice(ID3D11Device* Device);

		// Release resources that depend on the device.
		static void ReleaseDevice();
};

// This class creates a Direct3D 11 constant buffer

template <typename T>
class ConstantBuffer
{
public:

	// Constructor.
	ConstantBuffer() : Data(), mBuffer(NULL) {}
	// Destructor. Releases the Direct3D resource.
	~ConstantBuffer() { Release(); }

	// Copy of the data on the CPU side
	T Data;

	// Gets the Direct3D resource.
	ID3D11Buffer* GetBuffer() const { return mBuffer; }

	// Creates the Direct3D resource
	bool Create(ID3D11Device* Device)
	{
		int size = sizeof(T);
		if ((size & 15) != 0)	// take care of padding
		{
			size >>= 4;
			size++;
			size <<= 4;
		}

		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = size;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA init;
		ZeroMemory(&init, sizeof(D3D11_SUBRESOURCE_DATA));
		init.pSysMem = &Data;
		if (S_OK != Device->CreateBuffer(&desc, &init, &mBuffer))
			return false;

		return true;
	}

	// Releases the Direct3D resource
	void Release()
	{
		if (mBuffer) { mBuffer->Release(); mBuffer = NULL; }
	}

	// Maps the CPU content to the GPU
	void UpdateBuffer(ID3D11DeviceContext* ImmediateContext)
	{
		if (!mBuffer) return;
		D3D11_MAPPED_SUBRESOURCE MappedSubResource;
		ImmediateContext->Map(mBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource);
		*(T*)MappedSubResource.pData = Data;
		ImmediateContext->Unmap(mBuffer, 0);
	}

private:

	// Pointer to the Direct3D Resource
	ID3D11Buffer* mBuffer;

};

// Defines all properties
struct DynamicByteAddressBufferDesc
{
	DynamicByteAddressBufferDesc() : AccessFlag((D3D11_CPU_ACCESS_FLAG)0), BindFlags((D3D11_BIND_FLAG)0) {}
	DynamicByteAddressBufferDesc(int bindFlags, int accessFlag = 0) : AccessFlag(accessFlag), BindFlags(bindFlags) {}
	int AccessFlag;
	int BindFlags;
};

// a class that manages byte address buffers
template<typename T>
class DynamicByteAddressBuffer
{
public:
	DynamicByteAddressBuffer(const DynamicByteAddressBufferDesc& desc) : mDesc(desc), mBuffer(NULL), mDynamic(NULL), mStaging(NULL), mSrv(NULL), mUav(NULL), mReservedSizeInBytes(0)
	{
	}

	bool D3DCreate(ID3D11Device* device)
	{
		// if GPU buffer size is not large to handle the data, compute the size that we want to have
		if (mReservedSizeInBytes < Data.size() * sizeof(T))
			mReservedSizeInBytes = CalculateReservedSizeInBytes();

		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = mDesc.BindFlags;
		bufferDesc.ByteWidth = (UINT)mReservedSizeInBytes;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.MiscFlags = (((mDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0) || ((mDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0)) ? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : 0;

		std::vector<char> ini(mReservedSizeInBytes);
		memcpy_s(ini.data(), Data.size() * sizeof(T), Data.data(), Data.size() * sizeof(T));

		D3D11_SUBRESOURCE_DATA initData;
		ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
		initData.pSysMem = ini.data();
		if (FAILED(device->CreateBuffer(&bufferDesc, &initData, &mBuffer))) { return false; }

		if ((mDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
			uavDesc.Buffer.NumElements = (UINT)mReservedSizeInBytes / 4;
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			if (FAILED(device->CreateUnorderedAccessView(mBuffer, &uavDesc, &mUav))) return false;
		}

		if ((mDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			srvDesc.BufferEx.FirstElement = 0;
			srvDesc.BufferEx.NumElements = (UINT)mReservedSizeInBytes / 4;
			srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			if (FAILED(device->CreateShaderResourceView(mBuffer, &srvDesc, &mSrv))) return false;
		}

		// create the staging buffer
		if ((mDesc.AccessFlag & D3D11_CPU_ACCESS_READ) != 0)
		{
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			bufferDesc.Usage = D3D11_USAGE_STAGING;
			bufferDesc.BindFlags = 0;
			bufferDesc.MiscFlags = 0;
			if (FAILED(device->CreateBuffer(&bufferDesc, NULL, &mStaging))) return false;
		}

		// create the dynamic buffer
		if ((mDesc.AccessFlag & D3D11_CPU_ACCESS_WRITE) != 0)
		{
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.MiscFlags = 0;
			if (FAILED(device->CreateBuffer(&bufferDesc, NULL, &mDynamic))) return false;
		}

		return true;
	}

	void D3DRelease()
	{
		SAFE_RELEASE(mBuffer);
		SAFE_RELEASE(mDynamic);
		SAFE_RELEASE(mStaging);
		SAFE_RELEASE(mSrv);
		SAFE_RELEASE(mUav);
	}

	void ClearUav(ID3D11DeviceContext* immediateContext, const Vec4u& clearValue)
	{
		if (!mUav) return;
		unsigned int clearData[4] = { clearValue.x(), clearValue.y(), clearValue.z(), clearValue.w() };
		immediateContext->ClearUnorderedAccessViewUint(mUav, clearData);
	}

	ID3D11Buffer* GetBuffer() { return mBuffer; }
	ID3D11Buffer* GetDynamic() { return mDynamic; }
	ID3D11Buffer* GetStaging() { return mStaging; }
	ID3D11ShaderResourceView* GetSrv() { return mSrv; }
	ID3D11UnorderedAccessView* GetUav() { return mUav; }

	void CopyToCpu(ID3D11DeviceContext* immediateContext)
	{
		immediateContext->CopyResource(mStaging, mBuffer);

		int numel = (int)Data.size() * sizeof(T);
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(immediateContext->Map(mStaging, 0, D3D11_MAP_READ, 0, &mapped)))
		{
			memcpy_s(Data.data(), numel, mapped.pData, numel);
			immediateContext->Unmap(mStaging, 0);
		}
	}

	void CopyToGpu(ID3D11Device* device, ID3D11DeviceContext* immediateContext)
	{
		if (Data.size() * sizeof(T) < mReservedSizeInBytes)
		{
			int numel = (int)Data.size() * sizeof(T);
			D3D11_MAPPED_SUBRESOURCE mapped;
			if (SUCCEEDED(immediateContext->Map(mDynamic, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				memcpy_s(mapped.pData, numel, Data.data(), numel);
				immediateContext->Unmap(mDynamic, 0);
			}
			immediateContext->CopyResource(mBuffer, mDynamic);
		}
		else
		{
			D3DRelease();
			D3DCreate(device);
		}
	}

	std::vector<T> Data;

private:


	size_t CalculateReservedSizeInBytes() const { size_t i = 1; while (i < Data.size() * sizeof(T)) i *= 2; return i; }

	DynamicByteAddressBufferDesc mDesc;
	size_t mReservedSizeInBytes;

	ID3D11Buffer* mBuffer;
	ID3D11Buffer* mDynamic;
	ID3D11Buffer* mStaging;
	ID3D11ShaderResourceView* mSrv;
	ID3D11UnorderedAccessView* mUav;
};

typedef bool (*D3DCreateDevice_Callback)(ID3D11Device* Device, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc);
typedef bool (*D3DCreateSwapChain_Callback)(ID3D11Device* Device, IDXGISwapChain* SwapChain, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc);
typedef void (*D3DReleaseDevice_Callback)();
typedef void (*D3DReleaseSwapChain_Callback)();

// This class is responisble for the initialization of direct3d.
// Here dxgi factory, dxgi adapter, swap chain, device and immediate context are stored.
class D3D
{
	public:

		// Base constructor.
		explicit D3D(HWND WindowHandle);

		// Destructor.
		virtual ~D3D();

		// Initializes direct 3D (Creates device and swap chain.)
		bool Init();

		const D3D11_VIEWPORT& GetFullViewport() const { return _FullViewport; }
		
		ID3D11Device* GetDevice() { return _Device; }
		ID3D11DeviceContext* GetImmediateContext() { return _ImmediateContext; }
		ID3D11RenderTargetView* GetRtvBackbuffer() { return _RtvBackbuffer; }
		ID3D11DepthStencilView* GetDsvBackbuffer() { return _DsvBackbuffer; }
		ID3D11ShaderResourceView* GetSrvDepthbuffer() { return _SrvDepthbuffer; }
		HWND GetWindowHandle() { return _WindowHandle; }

		const DXGI_SURFACE_DESC& GetBackBufferSurfaceDesc() const { return _BackBufferSurfaceDesc; }
		IDXGISwapChain* GetSwapChain() { return _SwapChain; }

		bool CreateDevice();
		bool CreateSwapChain();

		void ReleaseDevice();
		void ReleaseSwapChain();

		void SetCreateDevice_Callback(D3DCreateDevice_Callback func) { _D3DCreateDevice_Callback = func; }
		void SetCreateSwapChain_Callback(D3DCreateSwapChain_Callback func) { _D3DCreateSwapChain_Callback = func; }
		void SetReleaseDevice_Callback(D3DReleaseDevice_Callback func) { _D3DReleaseDevice_Callback = func; }
		void SetReleaseSwapChain_Callback(D3DReleaseSwapChain_Callback func) { _D3DReleaseSwapChain_Callback = func; }
		
		void WaitForGPU(ID3D11DeviceContext* immediateContext);

		void ExportBackbufferToFile();
		void ExportBackbufferToFile(const char* path);

		static bool LoadEffectFromFile(const std::string& Filename, ID3D11Device* Device, ID3DX11Effect** Effect);
		static bool LoadComputeShaderFromFile(const std::string& Filename, ID3D11Device* Device, ID3D11ComputeShader** outShader);

	private:
		
		// Parent Window.
		HWND					_WindowHandle;

		IDXGIFactory*			_Factory;
		D3D_DRIVER_TYPE			_DriverType;		
		IDXGIAdapter*			_Adapter;
		ID3D11Device*			_Device;
		ID3D11DeviceContext*	_ImmediateContext;

		DXGI_SURFACE_DESC		_BackBufferSurfaceDesc;
		IDXGISwapChain*			_SwapChain;
		ID3D11RenderTargetView* _RtvBackbuffer;
		ID3D11Texture2D*		_TexBackbuffer;
		ID3D11Texture2D*		_TexBackbufferSingleSampled;
		ID3D11DepthStencilView* _DsvBackbuffer;
		ID3D11ShaderResourceView* _SrvDepthbuffer;
		D3D11_VIEWPORT			_FullViewport;		
		D3D_FEATURE_LEVEL		_FeatureLevel;		

		ID3D11Query*			_EventQuery;

		D3DCreateDevice_Callback _D3DCreateDevice_Callback;
		D3DCreateSwapChain_Callback _D3DCreateSwapChain_Callback;
		D3DReleaseDevice_Callback _D3DReleaseDevice_Callback;
		D3DReleaseSwapChain_Callback _D3DReleaseSwapChain_Callback;
};


// Creates a texture, a render target view and a shader resource view. (Currently CPU-side for R32 texture only)
class RenderTarget2D
{
public:

	// Constructor.
	RenderTarget2D(const std::string& DebugName, const DXGI_FORMAT& Format, bool HasUav = false);
	~RenderTarget2D();

	bool D3DCreateSwapChain(ID3D11Device* Device, IDXGISwapChain* SwapChain, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc);
	void D3DReleaseSwapChain();

	// Gets the shader resource view.
	inline ID3D11ShaderResourceView* GetSrv() { return _Srv; }
	// Gets the render target view.
	inline ID3D11RenderTargetView* GetRtv() { return _Rtv; }
	// Gets the underlying texture object.
	inline ID3D11Texture2D* GetTex() { return _Tex; }
	// Gets the unordered access view. Returns null, if no UAV is associated to this target. (You can do so by setting respective flag in the RenderTarget2D constructor.)
	inline ID3D11UnorderedAccessView* GetUav() { return _Uav; }
	// Checks whether an unordered access view is associated to this render target.
	inline const bool& HasUav() const { return _HasUav; }

	void CopyToCPU(ID3D11DeviceContext* ImmediateContext);
	void CopyToGPU(ID3D11DeviceContext* ImmediateContext);

	float& Get(int i, int j) { return _Data[j*_Resolution.x() + i]; }
	const float& Get(int i, int j) const { return _Data[j*_Resolution.x() + i]; }

	std::vector<float>& GetData() { return _Data; }
	Vec2i& GetResolution() { return _Resolution; }

private:

	Vec2i _Resolution;
	std::vector<float> _Data;

	// Direct3D texture.
	ID3D11Texture2D* _Tex;
	ID3D11Texture2D* _TexStaging;
	ID3D11Texture2D* _TexDynamic;
	// Direct3D render target view.
	ID3D11RenderTargetView* _Rtv;
	// Shader resource view.
	ID3D11ShaderResourceView* _Srv;
	// Unordered access view.
	ID3D11UnorderedAccessView* _Uav;

	// Debug name in PIX
	std::string _DebugName;

	// Internal format.
	DXGI_FORMAT _Format;

	// Flag that determines whether an UAV is created as well.
	bool _HasUav;
};
