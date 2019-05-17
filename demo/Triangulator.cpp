#include "stdafx.h"
#include "Triangulator.hpp"
#include "D3D.h"
#include "UI.h"
#include "MatlabInterface.hpp"
#include "ImmediateRenderer2D.h"
#include "Image2D.h"
#include <set>

extern std::string g_ResourcePath;
extern D3D* _D3D;
extern UI* _UI;


Triangulator::Triangulator(Image2DView* image, float refineVarianceThreshold) :
_Image(image),
mWireframeColor(0,0,0),
_FxRasterVariance(NULL),
_FxDisplayTriangulation(NULL),
_FxRasterPlane(NULL),
_ILTriangles(NULL),
_CsCopyToStaging(NULL),
_CsUpdatePosition(NULL),
_CsSolvePlane(NULL),
_CsSaturateTexture(NULL),
_StepSize(1),
_DrawMode(DrawMode_Color),
mBrushEventFrequency(0.1f),
_VarianceBounds(0, 10),
_IsBrushing(false),
mCursorPos(0,0),
_RefineIterations(100),
mPositions(DynamicByteAddressBufferDesc(D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE)),
mIndices(DynamicByteAddressBufferDesc(D3D11_BIND_INDEX_BUFFER | D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE)),
mTriAccumColor(DynamicByteAddressBufferDesc(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_CPU_ACCESS_WRITE)),
mTriVarianceColor(DynamicByteAddressBufferDesc(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_CPU_ACCESS_WRITE)),
mTriAccumColorSingle(DynamicByteAddressBufferDesc(D3D11_BIND_UNORDERED_ACCESS, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE)),
mTriVarianceColorSingle(DynamicByteAddressBufferDesc(D3D11_BIND_UNORDERED_ACCESS, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE)),
mAdjTriangleLists(DynamicByteAddressBufferDesc(D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE)),
mAdjTriangleEntry(DynamicByteAddressBufferDesc(D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE)),
mAdjTriangleCount(DynamicByteAddressBufferDesc(D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE)),
mTriPlaneAccumCoeffs(DynamicByteAddressBufferDesc(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_CPU_ACCESS_WRITE)),
mTriPlaneEquations(DynamicByteAddressBufferDesc(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, D3D11_CPU_ACCESS_WRITE)),
mRefineVarianceThreshold(refineVarianceThreshold),
mRefineNthIteration(1),
mDelaunayNthIteration(-1),
mEnableOptimization(true),
_FillMode(FillMode_Constant),
_InteractionState(Interaction_Pick),
_ShowWireframe(false),
mImmediateRenderer2D(new ImmediateRenderer2D()),
mBrushRadius(20),
mMaskDelaunayNumPoints(100),
_FxBrushing(NULL),
mMask(new RenderTarget2D("Mask", DXGI_FORMAT_R32_FLOAT, true))
{
	_CbParams.Data.UmbrellaDamping = 0.001f;
	_CbParams.Data.TrustRegion = 0.1f;

	Vec2i initialResolution(10, 10);
	ReadTriangulationFromMatlab(initialResolution);

	BuildNeighborhoodInformation();
	// encode into the neighbor list, which vertex index we are (0,1 or 2) inside the triangle
	BakeVertexIndexIntoAdjTriangleLists();
}

Triangulator::~Triangulator()
{
	SAFE_DELETE(mImmediateRenderer2D);
	SAFE_DELETE(mMask);
	D3DReleaseDevice();
}

void UI_CALL gClearSelection(void *clientData) { ((Triangulator*)clientData)->ClearSelection(); }
void UI_CALL gDelaunaySelection(void *clientData) { ((Triangulator*)clientData)->DelaunaySelection(); }

void UI_CALL gGetNumTriangles(void *value, void *clientData) { *((int*)value) = ((Triangulator*)clientData)->GetNumTriangles(); }
void UI_CALL gSetNumTriangles(const void *value, void *clientData) { }


void Triangulator::InitUI(UI* ui, UIBar* bar)
{
	// ---------- visualization parameters -----------
	int enumDrawMode = ui->CreateEnumType("EnumDrawMode", "Wireframe,Color,Variance,Tex");
	bar->AddVarEnum("DrawMode", "Draw Mode", &_DrawMode, "Visualization", "Selects what information to show.", enumDrawMode);
	bar->AddVarBool("ShowWireframe", "Show Wireframe", &_ShowWireframe, "Visualization", "Enables the visualization of the wireframe.");
	int enumFillMode = ui->CreateEnumType("EnumFillMode", "Constant,Linear");
	bar->AddVarEnum("FillMode", "Fill Mode", &_FillMode, "Visualization", "Toggles between constant triangle colors and linear color gradients.", enumFillMode);
//	bar->AddVarColor3("WireframeColor", "Wireframe Color", mWireframeColor.ptr(), "Visualization", "");
//	bar->AddVarFloat("TriVarBoundMin", "Variance Min", &_VarianceBounds.x(), -100000, 100000, "Visualization", "", 0.001f);
//	bar->AddVarFloat("TriVarBoundMax", "Variance Max", &_VarianceBounds.y(), -100000, 100000, "Visualization", "", 0.001f);

	// ---------- vertex optimization -----------
	bar->AddVarBool("EnableOptimization", "Optimize", &mEnableOptimization, "Vertex Optimization", "Enables the optimization of the triangle vertices via gradient descent.");
//	bar->AddVarFloat("TriStepSize", "Step Size", &_StepSize, 0, 100000, "Vertex Optimization", "", 0.00001f);
	bar->AddVarFloat("TriUmbrellaDamping", "Regularization", &_CbParams.Data.UmbrellaDamping, 0, 100000, "Vertex Optimization", "Adjusts how much regularization is added to avoid tiny needle triangles.", 0.00001f);
//	bar->AddVarFloat("TriTrustRegion", "Trust Region", &_CbParams.Data.TrustRegion, 0, 100000, "Vertex Optimization", "", 0.00001f);

	// ---------- triangle refinement -----------
	bar->AddVarInt("RefineIterations", "Refine Iterations", &_RefineIterations, 0, 10000000, "Refinement", "Sets the total number of refinement iterations that are applied after changing the variance threshold.");
	bar->AddVarFloat("RefineVarianceThreshold", "Variance Threshold", &mRefineVarianceThreshold, 0, 1000000000, "Refinement", "Sets the target approximation error.", 0.0001f);
	bar->AddVarInt("RefineNthIteration", "Refine Nth", &mRefineNthIteration, 0, 1000000, "Refinement", "Sets after how many iterations the triangles are checked for subdivisions.");
	bar->AddVarInt("DelaunayNthIteration", "Delaunay Nth", &mDelaunayNthIteration, 0, 1000000, "Refinement", "Sets after how many iterations the Delaunay triangulation is repaired.");
	bar->AddVarInt("NumberVertices", "Num Vertices", (const int*)&_CbParams.Data.NumVertices, 0, 1000000000, "Refinement", "Displays the total number of vertices of the current triangulation.");
	bar->AddVarInt("NumberTriangles", "Num Triangles", gSetNumTriangles, gGetNumTriangles, this, 0, 100000000, "Refinement", "Displays the total number of triangles of the current triangulation.");

	// ---------- manual editing -----------
	int enumInteractionMode = ui->CreateEnumType("EnumInteractionMode", "Pick,Brush,Select");
	bar->AddVarEnum("InteractionMode", "Interaction Mode", &_InteractionState, "Manual Editing", "Selects the user interaction method.", enumInteractionMode);
//	bar->AddVarInt("BrushRadius", "Brush Radius", &mBrushRadius, 0, 10000, "Manual Editing", "");
//	bar->AddVarFloat("BrushEventFrequency", "Brush Frequency", &mBrushEventFrequency, 0, 10000, "Manual Editing", "", 0.0001f);
	bar->AddButton("ClearSelection", "Clear Selection", gClearSelection, this, "Clears the selection created in 'Select' mode.", "Manual Editing", "c");
	bar->AddVarInt("MaskDelaunayNumPoints", "Delaunay Selection NumPoints", &mMaskDelaunayNumPoints, 0, 10000, "Manual Editing", "Sets how many points to add in the selected area.");
	bar->AddButton("DelaunaySelection", "Delaunay Selection", gDelaunaySelection, this, "Inserts the number of selected vertices.", "Manual Editing");
}


bool Triangulator::D3DCreateDevice(ID3D11Device* Device)
{
	if (!_CbParams.Create(Device)) return false;
	if (!mImmediateRenderer2D->D3DCreateDevice(Device)) return false;
	// load compute shaders
	if (!D3D::LoadComputeShaderFromFile("CopyToStaging.fxo", Device, &_CsCopyToStaging)) return false;
	if (!D3D::LoadComputeShaderFromFile("UpdatePosition.fxo", Device, &_CsUpdatePosition)) return false;
	if (!D3D::LoadComputeShaderFromFile("SolvePlane.fxo", Device, &_CsSolvePlane)) return false;
	if (!D3D::LoadComputeShaderFromFile("SaturateTexture.fxo", Device, &_CsSaturateTexture)) return false;

	// Create effect
	if (!D3D::LoadEffectFromFile("DisplayTriangulation.fxo", Device, &_FxDisplayTriangulation)) return false;
	if (!D3D::LoadEffectFromFile("RasterizeVariance.fxo", Device, &_FxRasterVariance)) return false;
	if (!D3D::LoadEffectFromFile("RasterizePlane.fxo", Device, &_FxRasterPlane)) return false;
	if (!D3D::LoadEffectFromFile("Brushing.fxo", Device, &_FxBrushing)) return false;

	// Create input layout
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3DX11_PASS_SHADER_DESC VsPassDesc;
	D3DX11_EFFECT_SHADER_DESC VsDesc;
	_FxDisplayTriangulation->GetTechniqueByName("RenderWire")->GetPassByIndex(0)->GetVertexShaderDesc(&VsPassDesc);
	VsPassDesc.pShaderVariable->GetShaderDesc(VsPassDesc.ShaderIndex, &VsDesc);
	if (FAILED(Device->CreateInputLayout(layout, 1, VsDesc.pBytecode, VsDesc.BytecodeLength, &_ILTriangles))) return false;

	if (!mPositions.D3DCreate(Device)) return false;
	if (!mIndices.D3DCreate(Device)) return false;
	
	// --- create resources for accumulated data per triangle (13 values!)
	{
		unsigned int NUM_ELEMENTS = GetNumTriangles() * 13;
		std::default_random_engine rng(1337);
		std::uniform_real_distribution<float> rnd(0, 1);
		mTriAccumColor.Data.resize(NUM_ELEMENTS);
		for (size_t i = 0; i < mTriAccumColor.Data.size(); ++i) {
			mTriAccumColor.Data[i].Data[0].Float = rnd(rng);
			mTriAccumColor.Data[i].Data[1].Float = rnd(rng);
			mTriAccumColor.Data[i].Data[2].Float = rnd(rng);
			mTriAccumColor.Data[i].Data[3].Float = 1;
		}
		if (!mTriAccumColor.D3DCreate(Device)) return false;
	}

	// --- create resources for staging the accumulated data back to CPU
	{
		unsigned int NUM_ELEMENTS = GetNumTriangles();
		mTriAccumColorSingle.Data.resize(NUM_ELEMENTS);
		if (!mTriAccumColorSingle.D3DCreate(Device)) return false;
		mTriVarianceColorSingle.Data.resize(NUM_ELEMENTS);
		if (!mTriVarianceColorSingle.D3DCreate(Device)) return false;
	}
	
	// --- create resources for variance data per triangle (13 values!)
	{
		unsigned int NUM_ELEMENTS = GetNumTriangles() * 13;
		mTriVarianceColor.Data.resize(NUM_ELEMENTS);
		std::default_random_engine rng;
		std::uniform_real_distribution<float> rnd(0, 1);
		for (size_t i = 0; i < mTriVarianceColor.Data.size(); ++i)
			for (size_t c = 0; c < 4; ++c)
			mTriVarianceColor.Data[i].Data[c].Float = (c % 4 == 3) ? 1 : rnd(rng) * 255;
		if (!mTriVarianceColor.D3DCreate(Device)) return false;
	}

	// --- create resources for planes
	{
		unsigned int NUM_ELEMENTS = GetNumTriangles() * 13;
		mTriPlaneAccumCoeffs.Data.resize(NUM_ELEMENTS);
		if (!mTriPlaneAccumCoeffs.D3DCreate(Device)) return false;
		mTriPlaneEquations.Data.resize(NUM_ELEMENTS);
		if (!mTriPlaneEquations.D3DCreate(Device)) return false;
	}
	
	// --- create resources for neighborhood lists
	{
		if (!mAdjTriangleLists.D3DCreate(Device)) return false;
	}

	// --- create resources for neighborhood list entries
	{
		if (!mAdjTriangleEntry.D3DCreate(Device)) return false;
	}

	// --- create resources for neighborhood list counter
	{
		if (!mAdjTriangleCount.D3DCreate(Device)) return false;
	}

	return true;
}

void Triangulator::D3DReleaseDevice()
{
	mPositions.D3DRelease();
	if (mImmediateRenderer2D) 
		mImmediateRenderer2D->D3DReleaseDevice();

	_CbParams.Release();

	mPositions.D3DRelease();
	mIndices.D3DRelease();
	mAdjTriangleLists.D3DRelease();
	mAdjTriangleEntry.D3DRelease();
	mAdjTriangleCount.D3DRelease();
	mTriAccumColor.D3DRelease();
	mTriAccumColorSingle.D3DRelease();
	mTriVarianceColor.D3DRelease();
	mTriVarianceColorSingle.D3DRelease();
	mTriPlaneAccumCoeffs.D3DRelease();
	mTriPlaneEquations.D3DRelease();
	
	SAFE_RELEASE(_CsCopyToStaging);
	SAFE_RELEASE(_CsUpdatePosition);
	SAFE_RELEASE(_CsSaturateTexture);
	SAFE_RELEASE(_FxRasterVariance);
	SAFE_RELEASE(_FxDisplayTriangulation);
	SAFE_RELEASE(_FxRasterPlane);
	SAFE_RELEASE(_FxBrushing);
	SAFE_RELEASE(_CsSolvePlane);
	SAFE_RELEASE(_ILTriangles);
}

bool Triangulator::D3DCreateSwapChain(ID3D11Device* Device, IDXGISwapChain* SwapChain, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc)
{
	if (!mMask->D3DCreateSwapChain(Device, SwapChain, BackBufferSurfaceDesc)) return false;
	return true;
}

void Triangulator::D3DReleaseSwapChain()
{
	if (mMask) mMask->D3DReleaseSwapChain();
}

void Triangulator::Draw(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D)
{
	switch (_DrawMode)
	{
	default:
	case DrawMode_Wireframe:
	case DrawMode_Color:
	case DrawMode_Variance:
	case DrawMode_Tex:
		DrawMode(_DrawMode, ImmediateContext, camera, D3D);
		break;
	}
	
	if (_ShowWireframe)
		DrawMode(DrawMode_Wireframe, ImmediateContext, camera, D3D);

	if (_InteractionState == Interaction_Brush  || _InteractionState == Interaction_Select)
	{
		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(_D3D->GetWindowHandle(), &cursor);
		cursor.y = (int)_D3D->GetFullViewport().Height - 1 - cursor.y;

		float u = (float)(2 * M_PI * mBrushRadius);
		int numSegments = (int)(u / 5);
		
		std::vector<Vec2f> points;
		for (size_t i = 0; i < numSegments; ++i)
		{
			float t = i / (numSegments - 1.f);
			points.push_back(Vec2f((float)(cos(2 * M_PI*t)*mBrushRadius + cursor.x), (float)(sin(2 * M_PI*t)*mBrushRadius + cursor.y)));
		}

		FLOAT factor[] = { 1,1,1,1 };
		ImmediateContext->OMSetBlendState(BlendStates::Invert(), factor, 0xFF);
		mImmediateRenderer2D->Color = Vec4f(1, 1, 1, 1);
		mImmediateRenderer2D->DrawLineStrip(points, _D3D->GetImmediateContext(), camera);
	}

	DrawAddToMask(ImmediateContext, camera, D3D);

	if (_InteractionState == Interaction_Select)
		DrawMaskContour(ImmediateContext, camera, D3D);
}

void Triangulator::DrawMode(EDrawMode drawMode, ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D)
{
	// Bind states
	ImmediateContext->IASetInputLayout(_ILTriangles);
	ID3D11RenderTargetView* rtvs[] = { D3D->GetRtvBackbuffer() };
	ImmediateContext->OMSetRenderTargets(1, rtvs, D3D->GetDsvBackbuffer());
	float blendFactor[4] = { 1,1,1,1 };
	ImmediateContext->OMSetBlendState(BlendStates::Default(), blendFactor, 0xffffffff);
	ImmediateContext->OMSetDepthStencilState(DepthStencilStates::DsTestWriteOff(), 0);

	switch (drawMode)
	{
	default:
	case DrawMode_Wireframe:
		ImmediateContext->RSSetState(RasterizerStates::WireCullNone());
		break;
	case DrawMode_Color:
		ImmediateContext->RSSetState(RasterizerStates::CullNone());
		if (_FillMode == FillMode_Constant) {
			_FxDisplayTriangulation->GetVariableByName("ColorData")->AsShaderResource()->SetResource(mTriAccumColor.GetSrv());
		}
		else if (_FillMode == FillMode_Linear) {
			_FxDisplayTriangulation->GetVariableByName("PlaneEquations")->AsShaderResource()->SetResource(mTriPlaneEquations.GetSrv());
		}
		break;
	case DrawMode_Variance:
		ImmediateContext->RSSetState(RasterizerStates::CullNone());
		_FxDisplayTriangulation->GetVariableByName("ColorData")->AsShaderResource()->SetResource(mTriVarianceColor.GetSrv());
		break;
	case DrawMode_Tex:
		ImmediateContext->RSSetState(RasterizerStates::CullNone());
		_FxDisplayTriangulation->GetVariableByName("ColorData")->AsShaderResource()->SetResource(mTriAccumColor.GetSrv());
		break;
	}
	int texResolution[] = { _Image->GetResolution().x(), _Image->GetResolution().y() };
	int screenResolution[] = { (int)D3D->GetBackBufferSurfaceDesc().Width, (int)D3D->GetBackBufferSurfaceDesc().Height };

	// Set effect parameters
	_FxDisplayTriangulation->GetVariableByName("View")->AsMatrix()->SetMatrix(camera->GetView().ptr());
	_FxDisplayTriangulation->GetVariableByName("Projection")->AsMatrix()->SetMatrix(camera->GetProj().ptr());
	//_FxDisplayTriangulation->GetVariableByName("Diffuse")->AsVector()->SetFloatVector(&_DiffuseColor.x);
	_FxDisplayTriangulation->GetVariableByName("TexResolution")->AsVector()->SetIntVector(texResolution);
	_FxDisplayTriangulation->GetVariableByName("ScreenResolution")->AsVector()->SetIntVector(screenResolution);
	_FxDisplayTriangulation->GetVariableByName("Diffuse")->AsVector()->SetFloatVector(mWireframeColor.ptr());
	_FxDisplayTriangulation->GetVariableByName("NumTriangles")->AsScalar()->SetInt(GetNumTriangles());

	// Bind shader (after setting effect parameters)
	switch (drawMode)
	{
	default:
	case DrawMode_Wireframe:
		_FxDisplayTriangulation->GetTechniqueByName("RenderWire")->GetPassByIndex(0)->Apply(0, ImmediateContext);
		break;
	case DrawMode_Color:
		if (_FillMode == FillMode_Constant) {
			_FxDisplayTriangulation->GetTechniqueByName("RenderColor")->GetPassByIndex(0)->Apply(0, ImmediateContext);
		}
		else if (_FillMode == FillMode_Linear) {
			_FxDisplayTriangulation->GetTechniqueByName("RenderColorPlane")->GetPassByIndex(0)->Apply(0, ImmediateContext);
		}
		break;
	case DrawMode_Variance:
		_FxDisplayTriangulation->GetVariableByName("VarianceBounds")->AsVector()->SetFloatVector(_VarianceBounds.ptr());

		if (_FillMode == FillMode_Linear)
			_FxDisplayTriangulation->GetVariableByName("VarianceScale")->AsScalar()->SetFloat(10);
		else _FxDisplayTriangulation->GetVariableByName("VarianceScale")->AsScalar()->SetFloat(1);
		_FxDisplayTriangulation->GetTechniqueByName("RenderVariance")->GetPassByIndex(0)->Apply(0, ImmediateContext);
		break;
	case DrawMode_Tex:
		_FxDisplayTriangulation->GetVariableByName("Image")->AsShaderResource()->SetResource(_Image->GetSrv());
		_FxDisplayTriangulation->GetTechniqueByName("RenderTex")->GetPassByIndex(0)->Apply(0, ImmediateContext);
		break;
	}	

	// setup input assembler
	ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11Buffer* vbs[] = { mPositions.GetBuffer() };
	UINT strides[] = { sizeof(XMFLOAT2) };
	UINT offsets[] = { 0 };
	ImmediateContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
	ImmediateContext->IASetIndexBuffer(mIndices.GetBuffer(), DXGI_FORMAT_R32_UINT, 0);

	// submit draw call
	ImmediateContext->DrawIndexed((UINT)mIndices.Data.size(), 0, 0);

	// unbind and clean up states
	ID3D11ShaderResourceView* noSrv[] = { NULL, NULL, NULL, NULL };
	ImmediateContext->GSSetShaderResources(0, 4, noSrv);
	ImmediateContext->PSSetShaderResources(0, 4, noSrv);
}

static LARGE_INTEGER timerLast, timerCurrent, frequency;
void Triangulator::DrawAddToMask(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D)
{
	if (_UI->IsUserInteracting() && _UI->IsMouseOver())
		return;

	QueryPerformanceCounter(&timerCurrent);
	QueryPerformanceFrequency(&frequency);
	LONGLONG timeElapsed = timerCurrent.QuadPart - timerLast.QuadPart;
	double elapsedS = (double)timeElapsed / (double)frequency.QuadPart;
	timerLast = timerCurrent;

	bool isMouseLeftDown = IsKeyDown(VK_LBUTTON);
	bool isMouseRightDown = IsKeyDown(VK_RBUTTON);
	if ((_InteractionState == Interaction_Select) && (isMouseLeftDown || isMouseRightDown))
	{
		float inkAmount = (float)(elapsedS * 5);
		if (isMouseRightDown)
			inkAmount *= -1;

		ID3D11RenderTargetView* rtvs[] = { mMask->GetRtv() };
		ImmediateContext->OMSetRenderTargets(1, rtvs, NULL);

		float factor[] = { 1,1,1,1 };
		ID3D11Buffer* noVBs[] = { NULL };
		unsigned int strides[] = { 0 };
		unsigned int offsets[] = { 0 };
		ImmediateContext->IASetVertexBuffers(0, 1, noVBs, strides, offsets);
		ImmediateContext->IASetInputLayout(NULL);
		ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		ImmediateContext->RSSetState(RasterizerStates::CullNone());
		ImmediateContext->OMSetBlendState(BlendStates::Additive(), factor, 0xFF);

		_FxBrushing->GetVariableByName("View")->AsMatrix()->SetMatrix(camera->GetView().ptr());
		_FxBrushing->GetVariableByName("Projection")->AsMatrix()->SetMatrix(camera->GetProj().ptr());
		float domainMin[] = { (float)(mCursorPos.x() - mBrushRadius), (float)(_D3D->GetFullViewport().Height - 1 - mCursorPos.y() - mBrushRadius) };
		float domainMax[] = { (float)(mCursorPos.x() + mBrushRadius), (float)(_D3D->GetFullViewport().Height - 1 - mCursorPos.y() + mBrushRadius) };
		_FxBrushing->GetVariableByName("DomainMin")->AsVector()->SetFloatVector(domainMin);
		_FxBrushing->GetVariableByName("DomainMax")->AsVector()->SetFloatVector(domainMax);
		_FxBrushing->GetVariableByName("InkAmount")->AsScalar()->SetFloat(inkAmount);
		_FxBrushing->GetTechniqueByName("ToDomainNormal")->GetPassByIndex(0)->Apply(0, ImmediateContext);

		ImmediateContext->Draw(4, 0);

		ID3D11RenderTargetView* nortvs[] = { NULL };
		ImmediateContext->OMSetRenderTargets(1, nortvs, D3D->GetDsvBackbuffer());

		{
			ImmediateContext->CSSetShader(_CsSaturateTexture, NULL, 0);

			ID3D11UnorderedAccessView* uavs[] = { mMask->GetUav() };
			UINT initialCounts[] = { 0, 0, 0, 0 };
			ImmediateContext->CSSetUnorderedAccessViews(0, 1, uavs, initialCounts);

			UINT groupsX = (UINT)_D3D->GetFullViewport().Width;
			UINT groupsY = (UINT)_D3D->GetFullViewport().Height;
			if (groupsX % (32) == 0)
				groupsX = groupsX / (32);
			else groupsX = groupsX / (32) + 1;
			if (groupsY % (32) == 0)
				groupsY = groupsY / (32);
			else groupsY = groupsY / (32) + 1;
			ImmediateContext->Dispatch(groupsX, groupsY, 1);

			// clean up
			ID3D11UnorderedAccessView* noUavs[] = { NULL };
			ImmediateContext->CSSetUnorderedAccessViews(0, 1, noUavs, initialCounts);
		}

		ID3D11RenderTargetView* rtvsb[] = { D3D->GetRtvBackbuffer() };
		ImmediateContext->OMSetRenderTargets(1, rtvsb, D3D->GetDsvBackbuffer());

	}
}

void Triangulator::DrawMaskContour(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D)
{
	float factor[] = { 1,1,1,1 };
	ID3D11Buffer* noVBs[] = { NULL };
	unsigned int strides[] = { 0 };
	unsigned int offsets[] = { 0 };
	ImmediateContext->IASetVertexBuffers(0, 1, noVBs, strides, offsets);
	ImmediateContext->IASetInputLayout(NULL);
	ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ImmediateContext->RSSetState(RasterizerStates::CullNone());
	ImmediateContext->OMSetBlendState(BlendStates::Invert(), factor, 0xFF);

	_FxBrushing->GetVariableByName("Image")->AsShaderResource()->SetResource(mMask->GetSrv());
	_FxBrushing->GetTechniqueByName("ToScreenIsolines")->GetPassByIndex(0)->Apply(0, ImmediateContext);
	ImmediateContext->Draw(4, 0);

	{
		ImmediateContext->OMSetBlendState(BlendStates::BlendOnBack(), factor, 0xFF);
		_FxBrushing->GetTechniqueByName("ToScreenBackground")->GetPassByIndex(0)->Apply(0, ImmediateContext);
		ImmediateContext->Draw(4, 0);
	}
	
	ID3D11ShaderResourceView* noSrv[1] = { NULL };
	ImmediateContext->PSSetShaderResources(0, 1, noSrv);
}

void Triangulator::ClearSelection()
{
	float clear[] = { 0,0,0,0 };
	_D3D->GetImmediateContext()->ClearRenderTargetView(mMask->GetRtv(), clear);
}

void Triangulator::DelaunaySelection()
{
	// copy mask data to CPU
	mMask->CopyToCPU(_D3D->GetImmediateContext());

	// send mask to Matlab
	int w = (int)(_D3D->GetFullViewport().Width);
	int h = (int)(_D3D->GetFullViewport().Height);
	Eigen::MatrixXd image(h, w);
	for (int i = 0; i < w; ++i)
		for (int j = 0; j < h; ++j)
			image(j, i) = mMask->Get(i, j);
	util::MatlabInterface::putEigenMatrix("Mask", image);

	// copy positions to CPU
	SendDataToCPU(_D3D->GetImmediateContext());
	WritePositionsToMatlab("points");
	WriteIndicesToMatlab("triangles");

	// set parameters
	util::MatlabInterface::putScalar("NumRandomPoints", mMaskDelaunayNumPoints);
	
	// call the matlab script
	auto s = util::MatlabInterface::meval("[points, triangles] = updateTriangulationBasedOnMask(Mask, NumRandomPoints, points, triangles);");

	// read data from matlab
	ReadPositionsFromMatlab("points");
	ReadIndicesFromMatlab("triangles");

	// send data to GPU
	SendDataToGPU(_D3D->GetDevice(), _D3D->GetImmediateContext());
}

void Triangulator::Iterate(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D)
{
	if (!_UI->IsUserInteracting())
	{
		if (IsKeyPressed('1'))
			_InteractionState = Interaction_Pick;
		if (IsKeyPressed('2'))
			_InteractionState = Interaction_Brush;
		if (IsKeyPressed('3'))
			_InteractionState = Interaction_Select;
	}
	if (IsKeyPressed(' '))
		mEnableOptimization = !mEnableOptimization;
	if (IsKeyPressed('R'))
		mRefineNthIteration *= -1;
	if (IsKeyPressed('D'))
		mDelaunayNthIteration *= -1;

	if (IsKeyPressed(VK_F1)) {
		_DrawMode = DrawMode_Color;
		_FillMode = FillMode_Constant;
	}
	else if (IsKeyPressed(VK_F2)) {
		_DrawMode = DrawMode_Color;
		_FillMode = FillMode_Linear;
	}
	
	if (IsKeyPressed('W')) {
		_ShowWireframe = !_ShowWireframe;
	}

	static int iterationNr = 0;
	iterationNr++;
	bool hasChanged = false;
	if (iterationNr > 1)
	{
		if ((mRefineNthIteration > 0 && (iterationNr % mRefineNthIteration == 0))
			|| (mDelaunayNthIteration > 0 && (iterationNr % mDelaunayNthIteration == 0)))
		SendDataToCPU(ImmediateContext);

		static float lastThreshold = -1;
		static int lastCount = 0;

		if (lastThreshold != mRefineVarianceThreshold) {
			lastThreshold = mRefineVarianceThreshold;
			lastCount = _RefineIterations;
		}
		
		if (mRefineNthIteration > 0 && (iterationNr % mRefineNthIteration == 0) 
			&& lastCount > 0)
		{
			lastCount--;

			IdentifyTrianglesToRefineThreshold();
			RefineMatlab();
			
			hasChanged = true;
		}

		if (mDelaunayNthIteration > 0 && (iterationNr % mDelaunayNthIteration == 0))
		{
			WritePositionsToMatlab("pos");
			auto s1 = util::MatlabInterface::meval("[tris]=delaunay(pos(1,:),pos(2,:))';");
			ReadIndicesFromMatlab("tris");
			hasChanged = true;
		}

		if (hasChanged)
			SendDataToGPU(D3D->GetDevice(), ImmediateContext);
	}

	{
		if (_FillMode == FillMode_Constant)
			ComputeConstVariance(ImmediateContext, camera, D3D);
		else if (_FillMode == FillMode_Linear)
			ComputePlaneVariance(ImmediateContext, camera, D3D);
		
		// sum up the variances around the vertices and move the vertex to a better position
		if (mEnableOptimization && !_IsBrushing)
			UpdatePositions(ImmediateContext);
	}

}

void Triangulator::ComputeConstVariance(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D)
{
	// clear
	{
		// Clear the start offset buffer by magic value.
		Vec4u clearStartOffset(0, 0, 0, 0);
		mTriAccumColor.ClearUav(ImmediateContext, clearStartOffset);
		mTriVarianceColor.ClearUav(ImmediateContext, clearStartOffset);
	}

	D3D11_VIEWPORT viewport = D3D->GetFullViewport();
	viewport.Width = (float)_Image->GetResolution().x();
	viewport.Height = (float)_Image->GetResolution().y();
	ImmediateContext->RSSetViewports(1, &viewport);

	// rasterize variance
	{
		// Bind states
		ImmediateContext->IASetInputLayout(_ILTriangles);
		ID3D11RenderTargetView* rtvsNo[] = { NULL };
		ImmediateContext->OMSetRenderTargets(0, rtvsNo, NULL);
		float blendFactor[4] = { 1,1,1,1 };
		ImmediateContext->OMSetBlendState(BlendStates::Default(), blendFactor, 0xffffffff);
		ImmediateContext->OMSetDepthStencilState(DepthStencilStates::DsTestWriteOff(), 0);
		ImmediateContext->RSSetState(RasterizerStates::CullNone());

		// Set effect parameters
		_FxRasterVariance->GetVariableByName("View")->AsMatrix()->SetMatrix(camera->GetView().ptr());
		_FxRasterVariance->GetVariableByName("Projection")->AsMatrix()->SetMatrix(camera->GetProj().ptr());
		_FxRasterVariance->GetVariableByName("Image")->AsShaderResource()->SetResource(_Image->GetSrv());
		int screenResolution[] = { (int)viewport.Width, (int)viewport.Height };
		_FxRasterVariance->GetVariableByName("ScreenResolution")->AsVector()->SetIntVector(screenResolution);

		// Bind shader (after setting effect parameters)
		_FxRasterVariance->GetTechniqueByName("RenderMean")->GetPassByIndex(0)->Apply(0, ImmediateContext);

		ID3D11UnorderedAccessView* uavs[] = { mTriAccumColor.GetUav() };
		UINT initialCount[] = { 0,0,0,0 };
		// render target is only bound so that the rasterizer knows, how many samples we want. We won't render into it, yet.
		ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 1, uavs, initialCount);

		// setup input assembler
		ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ID3D11Buffer* vbs[] = { mPositions.GetBuffer() };
		UINT strides[] = { sizeof(XMFLOAT2) };
		UINT offsets[] = { 0 };
		ImmediateContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
		ImmediateContext->IASetIndexBuffer(mIndices.GetBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// submit draw call
		ImmediateContext->DrawIndexed((UINT)mIndices.Data.size(), 0, 0);

		// unbind and clean up states
		ID3D11ShaderResourceView* noSrv[] = { NULL, NULL, NULL, NULL };
		ImmediateContext->PSSetShaderResources(0, 1, noSrv);

		{
			ID3D11UnorderedAccessView* uavs[] = { NULL, NULL, NULL };
			UINT initialCount[] = { 0,0,0 };
			ID3D11RenderTargetView* rtvsNo[] = { NULL };
			ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 3, uavs, initialCount);
		}
	}

	{
		// Bind states
		ImmediateContext->IASetInputLayout(_ILTriangles);
		ID3D11RenderTargetView* rtvsNo[] = { NULL };
		ImmediateContext->OMSetRenderTargets(0, rtvsNo, NULL);
		float blendFactor[4] = { 1,1,1,1 };
		ImmediateContext->OMSetBlendState(BlendStates::Default(), blendFactor, 0xffffffff);
		ImmediateContext->OMSetDepthStencilState(DepthStencilStates::DsTestWriteOff(), 0);
		ImmediateContext->RSSetState(RasterizerStates::CullNone());

		// Set effect parameters
		_FxRasterVariance->GetVariableByName("View")->AsMatrix()->SetMatrix(camera->GetView().ptr());
		_FxRasterVariance->GetVariableByName("Projection")->AsMatrix()->SetMatrix(camera->GetProj().ptr());
		_FxRasterVariance->GetVariableByName("Image")->AsShaderResource()->SetResource(_Image->GetSrv());
		int screenResolution[] = { (int)viewport.Width, (int)viewport.Height };
		_FxRasterVariance->GetVariableByName("ScreenResolution")->AsVector()->SetIntVector(screenResolution);
		_FxRasterVariance->GetVariableByName("SrvAccumColor")->AsShaderResource()->SetResource(mTriAccumColor.GetSrv());
		
		// Bind shader (after setting effect parameters)
		_FxRasterVariance->GetTechniqueByName("RenderVariance")->GetPassByIndex(0)->Apply(0, ImmediateContext);

		ID3D11UnorderedAccessView* uavs[] = { mTriVarianceColor.GetUav() };
		UINT initialCount[] = { 0,0,0,0 };
		// render target is only bound so that the rasterizer knows, how many samples we want. We won't render into it, yet.
		ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 1, uavs, initialCount);

		// setup input assembler
		ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ID3D11Buffer* vbs[] = { mPositions.GetBuffer() };
		UINT strides[] = { sizeof(XMFLOAT2) };
		UINT offsets[] = { 0 };
		ImmediateContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
		ImmediateContext->IASetIndexBuffer(mIndices.GetBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// submit draw call
		ImmediateContext->DrawIndexed((UINT)mIndices.Data.size(), 0, 0);

		// unbind and clean up states
		ID3D11ShaderResourceView* noSrv[] = { NULL, NULL, NULL, NULL };
		ImmediateContext->PSSetShaderResources(0, 1, noSrv);

		{
			ID3D11UnorderedAccessView* uavs[] = { NULL, NULL, NULL };
			UINT initialCount[] = { 0,0,0 };
			ID3D11RenderTargetView* rtvsNo[] = { NULL };
			ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 3, uavs, initialCount);
		}

		ID3D11Buffer* noVbs[] = { NULL };
		ImmediateContext->IASetVertexBuffers(0, 1, noVbs, strides, offsets);
	}

	ImmediateContext->RSSetViewports(1, &D3D->GetFullViewport());
}

void Triangulator::ComputePlaneVariance(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D)
{
	// clear
	{
		// Clear the start offset buffer by magic value.
		Vec4u clearStartOffset(0, 0, 0, 0);
		mTriPlaneAccumCoeffs.ClearUav(ImmediateContext, clearStartOffset);
		mTriPlaneEquations.ClearUav(ImmediateContext, clearStartOffset);
		mTriVarianceColor.ClearUav(ImmediateContext, clearStartOffset);
	}

	D3D11_VIEWPORT viewport = D3D->GetFullViewport();
	viewport.Width = (float)_Image->GetResolution().x();
	viewport.Height = (float)_Image->GetResolution().y();
	ImmediateContext->RSSetViewports(1, &viewport);

	// rasterize variance
	{
		// Bind states
		ImmediateContext->IASetInputLayout(_ILTriangles);
		ID3D11RenderTargetView* rtvsNo[] = { NULL };
		ImmediateContext->OMSetRenderTargets(0, rtvsNo, NULL);
		float blendFactor[4] = { 1,1,1,1 };
		ImmediateContext->OMSetBlendState(BlendStates::Default(), blendFactor, 0xffffffff);
		ImmediateContext->OMSetDepthStencilState(DepthStencilStates::DsTestWriteOff(), 0);
		ImmediateContext->RSSetState(RasterizerStates::CullNone());

		// Set effect parameters
		_FxRasterPlane->GetVariableByName("View")->AsMatrix()->SetMatrix(camera->GetView().ptr());
		_FxRasterPlane->GetVariableByName("Projection")->AsMatrix()->SetMatrix(camera->GetProj().ptr());
		_FxRasterPlane->GetVariableByName("Image")->AsShaderResource()->SetResource(_Image->GetSrv());
		int screenResolution[] = { (int)viewport.Width, (int)viewport.Height };
		int texResolution[] = { _Image->GetResolution().x(), _Image->GetResolution().y() };
		_FxRasterPlane->GetVariableByName("ScreenResolution")->AsVector()->SetIntVector(screenResolution);
		_FxRasterPlane->GetVariableByName("TexResolution")->AsVector()->SetIntVector(texResolution);

		// Bind shader (after setting effect parameters)
		_FxRasterPlane->GetTechniqueByName("RenderCoefficients")->GetPassByIndex(0)->Apply(0, ImmediateContext);

		ID3D11UnorderedAccessView* uavs[] = { mTriPlaneAccumCoeffs.GetUav() };
		UINT initialCount[] = { 0,0,0,0 };
		// render target is only bound so that the rasterizer knows, how many samples we want. We won't render into it, yet.
		ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 1, uavs, initialCount);

		// setup input assembler
		ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ID3D11Buffer* vbs[] = { mPositions.GetBuffer() };
		UINT strides[] = { sizeof(XMFLOAT2) };
		UINT offsets[] = { 0 };
		ImmediateContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
		ImmediateContext->IASetIndexBuffer(mIndices.GetBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// submit draw call
		ImmediateContext->DrawIndexed((UINT)mIndices.Data.size(), 0, 0);

		// unbind and clean up states
		ID3D11ShaderResourceView* noSrv[] = { NULL, NULL, NULL, NULL };
		ImmediateContext->PSSetShaderResources(0, 1, noSrv);

		{
			ID3D11UnorderedAccessView* uavs[] = { NULL, NULL, NULL };
			UINT initialCount[] = { 0,0,0 };
			ID3D11RenderTargetView* rtvsNo[] = { NULL };
			ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 3, uavs, initialCount);
		}
	}

	// solve planes
	{
		ImmediateContext->CSSetShader(_CsSolvePlane, NULL, 0);

		ID3D11ShaderResourceView* srvs[] = { mTriPlaneAccumCoeffs.GetSrv() };
		ImmediateContext->CSSetShaderResources(0, 1, srvs);

		ID3D11UnorderedAccessView* uavs[] = { mTriPlaneEquations.GetUav() };
		UINT initialCounts[] = { 0, 0, 0, 0 };
		ImmediateContext->CSSetUnorderedAccessViews(0, 1, uavs, initialCounts);

		UINT groupsX = GetNumTriangles() * 13;
		if (groupsX % (512) == 0)
			groupsX = groupsX / (512);
		else groupsX = groupsX / (512) + 1;
		ImmediateContext->Dispatch(groupsX, 1, 1);

		// clean up
		ID3D11ShaderResourceView* noSrvs[] = { NULL };
		ImmediateContext->CSSetShaderResources(0, 1, noSrvs);

		ID3D11UnorderedAccessView* noUavs[] = { NULL };
		ImmediateContext->CSSetUnorderedAccessViews(0, 1, noUavs, initialCounts);

		ID3D11Buffer* noCbs[] = { NULL };
		ImmediateContext->CSSetConstantBuffers(0, 1, noCbs);
	}

	// draw mean and error
	{
		// Bind states
		ImmediateContext->IASetInputLayout(_ILTriangles);
		ID3D11RenderTargetView* rtvsNo[] = { NULL };
		ImmediateContext->OMSetRenderTargets(0, rtvsNo, NULL);
		float blendFactor[4] = { 1,1,1,1 };
		ImmediateContext->OMSetBlendState(BlendStates::Default(), blendFactor, 0xffffffff);
		ImmediateContext->OMSetDepthStencilState(DepthStencilStates::DsTestWriteOff(), 0);
		ImmediateContext->RSSetState(RasterizerStates::CullNone());

		// Set effect parameters
		_FxRasterPlane->GetVariableByName("View")->AsMatrix()->SetMatrix(camera->GetView().ptr());
		_FxRasterPlane->GetVariableByName("Projection")->AsMatrix()->SetMatrix(camera->GetProj().ptr());
		_FxRasterPlane->GetVariableByName("Image")->AsShaderResource()->SetResource(_Image->GetSrv());
		int screenResolution[] = { (int)viewport.Width, (int)viewport.Height };
		int texResolution[] = { _Image->GetResolution().x(), _Image->GetResolution().y() };
		_FxRasterPlane->GetVariableByName("ScreenResolution")->AsVector()->SetIntVector(screenResolution);
		_FxRasterPlane->GetVariableByName("TexResolution")->AsVector()->SetIntVector(texResolution);
		_FxRasterPlane->GetVariableByName("SrvEquations")->AsShaderResource()->SetResource(mTriPlaneEquations.GetSrv());

		// Bind shader (after setting effect parameters)
		_FxRasterPlane->GetTechniqueByName("RenderMeanError")->GetPassByIndex(0)->Apply(0, ImmediateContext);

		ID3D11UnorderedAccessView* uavs[] = { mTriVarianceColor.GetUav() };
		UINT initialCount[] = { 0,0,0,0 };
		// render target is only bound so that the rasterizer knows, how many samples we want. We won't render into it, yet.
		ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 1, uavs, initialCount);

		// setup input assembler
		ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ID3D11Buffer* vbs[] = { mPositions.GetBuffer() };
		UINT strides[] = { sizeof(XMFLOAT2) };
		UINT offsets[] = { 0 };
		ImmediateContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
		ImmediateContext->IASetIndexBuffer(mIndices.GetBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// submit draw call
		ImmediateContext->DrawIndexed((UINT)mIndices.Data.size(), 0, 0);

		// unbind and clean up states
		ID3D11ShaderResourceView* noSrv[] = { NULL, NULL, NULL, NULL };
		ImmediateContext->PSSetShaderResources(0, 1, noSrv);

		{
			ID3D11UnorderedAccessView* uavs[] = { NULL, NULL, NULL };
			UINT initialCount[] = { 0,0,0 };
			ID3D11RenderTargetView* rtvsNo[] = { NULL };
			ImmediateContext->OMSetRenderTargetsAndUnorderedAccessViews(1, rtvsNo, NULL, 1, 3, uavs, initialCount);
		}
	}

	{
		UINT strides[] = { sizeof(XMFLOAT2) };
		UINT offsets[] = { 0 };
		ID3D11Buffer* noVbs[] = { NULL };
		ImmediateContext->IASetVertexBuffers(0, 1, noVbs, strides, offsets);
	}
	
	ImmediateContext->RSSetViewports(1, &D3D->GetFullViewport());
}

void Triangulator::ReadTriangulationFromMatlab(const Vec2i& res)
{
	Vec2f dims = _Image->GetBoundingBox().GetMax() - _Image->GetBoundingBox().GetMin();

	// add path variable
	util::MatlabInterface::meval("addpath('"+ g_ResourcePath+"');");

	// set parameters
	util::MatlabInterface::putScalar("dimX", dims.x());
	util::MatlabInterface::putScalar("dimY", dims.y());
	util::MatlabInterface::putScalar("numberOfPoints_X", res.x());
	util::MatlabInterface::putScalar("numberOfPoints_Y", res.y());

	// call the matlab script
	auto s = util::MatlabInterface::meval("[points,triangles,neighborList,neighborStartIndex,numberOfNeighbors]=getRegularGrid(dimX,dimY,numberOfPoints_X,numberOfPoints_Y);");

	// read the matrices back
	Eigen::MatrixXd points = util::MatlabInterface::getEigenMatrix<double, double>("points");
	Eigen::MatrixXd triangles = util::MatlabInterface::getEigenMatrix<double, double>("triangles");
	Eigen::MatrixXd neighborList = util::MatlabInterface::getEigenMatrix<double, double>("neighborList");
	Eigen::MatrixXd neighborStartIndex = util::MatlabInterface::getEigenMatrix<double, double>("neighborStartIndex");
	Eigen::MatrixXd numberOfNeighbors = util::MatlabInterface::getEigenMatrix<double, double>("numberOfNeighbors");
	
	int c = (int)points.cols();
	int r = (int)points.rows();

	ReadPositionsFromMatlab("points");
	ReadIndicesFromMatlab("triangles");

	mAdjTriangleLists.Data.resize(neighborList.cols());
	for (int c = 0; c < neighborList.cols(); ++c)
		mAdjTriangleLists.Data[c] = (int)(neighborList(0, c) + 0.00000001) - 1;

	mAdjTriangleEntry.Data.resize(neighborStartIndex.cols());
	for (int c = 0; c < neighborStartIndex.cols(); ++c)
		mAdjTriangleEntry.Data[c] = (int)(neighborStartIndex(0, c) + 0.00000001) - 1;

	mAdjTriangleCount.Data.resize(numberOfNeighbors.cols());
	for (int c = 0; c < numberOfNeighbors.cols(); ++c)
		mAdjTriangleCount.Data[c] = (int)(numberOfNeighbors(0, c) + 0.00000001);
}

void Triangulator::ReadPositionsFromMatlab(const std::string& positionVar)
{
	// read the matrices back
	Eigen::MatrixXd points = util::MatlabInterface::getEigenMatrix<double, double>(positionVar);
	
	Vec2f dims = _Image->GetBoundingBox().GetMax() - _Image->GetBoundingBox().GetMin();
	int c = (int)points.cols();
	int r = (int)points.rows();

	mPositions.Data.resize(points.cols());
	for (int c = 0; c < points.cols(); ++c)
	{
		mPositions.Data[c] = Vec2f((float)points(0, c), (float)points(1, c));
		mPositions.Data[c].x() = (mPositions.Data[c].x() - 1) / (dims.x() - 1) * dims.x();	// positions were scaled from [1, dims] -> transform to [0, dims]
		mPositions.Data[c].y() = (1 - (mPositions.Data[c].y() - 1) / (dims.y() - 1)) * dims.y();
	}
}

void Triangulator::ReadIndicesFromMatlab(const std::string& indexVar)
{
	// read the matrices back
	Eigen::MatrixXd triangles = util::MatlabInterface::getEigenMatrix<double, double>(indexVar);

	mIndices.Data.resize(triangles.cols()*triangles.rows());
	for (int c = 0; c < triangles.cols(); ++c)
	{
		mIndices.Data[c * 3 + 0] = (int)(triangles(0, c) + 0.00000001) - 1;
		mIndices.Data[c * 3 + 1] = (int)(triangles(1, c) + 0.00000001) - 1;
		mIndices.Data[c * 3 + 2] = (int)(triangles(2, c) + 0.00000001) - 1;
	}
}

void Triangulator::WritePositionsToMatlab(const std::string& positionVar)
{
	Vec2f dims = _Image->GetBoundingBox().GetMax() - _Image->GetBoundingBox().GetMin();
	// read the matrices back
	Eigen::MatrixXd points(2, mPositions.Data.size());
	for (size_t i = 0; i < mPositions.Data.size(); ++i)
	{
		Vec2f pnt = mPositions.Data[i];
		points(0, i) = pnt.x() / dims.x() * (dims.x() - 1) + 1;
		points(1, i) = (1 - pnt.y() / dims.y()) * (dims.y() - 1) + 1;
	}
	util::MatlabInterface::putEigenMatrix<Eigen::MatrixXd>(positionVar, points);
}

void Triangulator::WriteIndicesToMatlab(const std::string& indexVar)
{
	// read the matrices back
	Eigen::MatrixXd triangles(3, mIndices.Data.size() / 3);
	int numTris = GetNumTriangles();
	for (int i = 0; i < numTris; ++i)
	{
		triangles(0, i) = mIndices.Data[3 * i + 0] + 1;
		triangles(1, i) = mIndices.Data[3 * i + 1] + 1;
		triangles(2, i) = mIndices.Data[3 * i + 2] + 1;
	}
	util::MatlabInterface::putEigenMatrix<Eigen::MatrixXd>(indexVar, triangles);
}

void Triangulator::WriteIntVectorToMatlab(const std::string& vectorVar, const std::vector<int>& vec)
{
	// read the matrices back
	Eigen::MatrixXd triangles(1, vec.size());
	for (size_t i = 0; i < vec.size(); ++i)
	{
		triangles(0, i) = vec[i];
	}
	util::MatlabInterface::putEigenMatrix<Eigen::MatrixXd>(vectorVar, triangles);
}

void Triangulator::CopyTriangleColorsToCPU(ID3D11DeviceContext* immediateContext, bool accumulatedColor, bool varianceColor)
{
	if (accumulatedColor)
	{
		mTriAccumColorSingle.Data.resize(GetNumTriangles());
		CopyTriangleColorsToSingle(mTriAccumColor.GetSrv(), mTriAccumColorSingle.GetUav(), immediateContext);
		mTriAccumColorSingle.CopyToCpu(immediateContext);
	}
		
	if (varianceColor)
	{
		mTriVarianceColorSingle.Data.resize(GetNumTriangles());
		CopyTriangleColorsToSingle(mTriVarianceColor.GetSrv(), mTriVarianceColorSingle.GetUav(), immediateContext);
		mTriVarianceColorSingle.CopyToCpu(immediateContext);
	}
}

void Triangulator::CopyTriangleColorsToSingle(ID3D11ShaderResourceView* srvToCopy, ID3D11UnorderedAccessView* uavTarget, ID3D11DeviceContext* immediateContext)
{
	immediateContext->CSSetShader(_CsCopyToStaging, NULL, 0);

	ID3D11ShaderResourceView* srvs[] = { srvToCopy };
	immediateContext->CSSetShaderResources(0, 1, srvs);

	ID3D11UnorderedAccessView* uavs[] = { uavTarget };
	UINT initialCounts[] = { 0, 0, 0, 0 };
	immediateContext->CSSetUnorderedAccessViews(0, 1, uavs, initialCounts);

	UINT groupsX = GetNumTriangles();
	if (groupsX % (512) == 0)
		groupsX = groupsX / (512);
	else groupsX = groupsX / (512) + 1;
	immediateContext->Dispatch(groupsX, 1, 1);

	// clean up
	ID3D11ShaderResourceView* noSrvs[] = { NULL };
	immediateContext->CSSetShaderResources(0, 1, noSrvs);

	ID3D11UnorderedAccessView* noUavs[] = { NULL };
	immediateContext->CSSetUnorderedAccessViews(0, 1, noUavs, initialCounts);

	ID3D11Buffer* noCbs[] = { NULL };
	immediateContext->CSSetConstantBuffers(0, 1, noCbs);
}

void Triangulator::CopyPositionsToCPU(ID3D11DeviceContext* immediateContext)
{
	mPositions.CopyToCpu(immediateContext);
}

void Triangulator::UpdatePositions(ID3D11DeviceContext* immediateContext)
{
	// set shader
	immediateContext->CSSetShader(_CsUpdatePosition, NULL, 0);

	// set constants
	_CbParams.Data.StepSize = _StepSize;
	_CbParams.Data.NumVertices = (int)mPositions.Data.size();
	_CbParams.Data.DomainSize = _Image->GetBoundingBox().GetMax() - _Image->GetBoundingBox().GetMin();
	ID3D11Buffer* cbs[] = { _CbParams.GetBuffer() };
	immediateContext->CSSetConstantBuffers(0, 1, cbs);
	_CbParams.UpdateBuffer(immediateContext);

	// bind srvs
	ID3D11ShaderResourceView* srvs[] = { mAdjTriangleEntry.GetSrv(), mAdjTriangleCount.GetSrv(), mAdjTriangleLists.GetSrv(), mTriVarianceColor.GetSrv(), mIndices.GetSrv() };
	immediateContext->CSSetShaderResources(0, 5, srvs);

	// bind uavs
	ID3D11UnorderedAccessView* uavs[] = { mPositions.GetUav() };
	UINT initialCounts[] = { 0, 0, 0, 0 };
	immediateContext->CSSetUnorderedAccessViews(0, 1, uavs, initialCounts);

	// dispatch threads
	UINT groupsX = (UINT) std::max(0, (int)mPositions.Data.size());
	if (groupsX % (512) == 0)
		groupsX = groupsX / (512);
	else groupsX = groupsX / (512) + 1;
	immediateContext->Dispatch(groupsX, 1, 1);

	// clean up
	ID3D11ShaderResourceView* noSrvs[] = { NULL, NULL, NULL, NULL, NULL };
	immediateContext->CSSetShaderResources(0, 5, noSrvs);

	ID3D11UnorderedAccessView* noUavs[] = { NULL };
	immediateContext->CSSetUnorderedAccessViews(0, 1, noUavs, initialCounts);

	ID3D11Buffer* noCbs[] = { NULL };
	immediateContext->CSSetConstantBuffers(0, 1, noCbs);
}

void Triangulator::BuildNeighborhoodInformation()
{
	std::vector<std::list<int>> adjacentTris;
	adjacentTris.resize(mPositions.Data.size());
	for (int i = 0; i < GetNumTriangles(); ++i)
	{
		int v0 = mIndices.Data[3 * i + 0];
		int v1 = mIndices.Data[3 * i + 1];
		int v2 = mIndices.Data[3 * i + 2];
		adjacentTris[v0].push_back(i);
		adjacentTris[v1].push_back(i);
		adjacentTris[v2].push_back(i);
	}

	mAdjTriangleCount.Data.resize(mPositions.Data.size());
	mAdjTriangleEntry.Data.resize(mPositions.Data.size());
	mAdjTriangleLists.Data.resize(GetNumTriangles() * 3);
	int adj = 0;
	for (size_t v = 0; v < mPositions.Data.size(); ++v)
	{
		mAdjTriangleEntry.Data[v] = adj;
		mAdjTriangleCount.Data[v] = (unsigned int)adjacentTris[v].size();
		for (auto& tri : adjacentTris[v])
			mAdjTriangleLists.Data[adj++] = tri;
	}
}

void Triangulator::BakeVertexIndexIntoAdjTriangleLists()
{
	mAdjTriangleEntry.Data.resize(mPositions.Data.size());
	mAdjTriangleCount.Data.resize(mPositions.Data.size());
	mAdjTriangleLists.Data.resize(GetNumTriangles() * 3);
	for (size_t v = 0; v < mPositions.Data.size(); ++v)
	{
		unsigned int entry = mAdjTriangleEntry.Data[v];
		unsigned int count = mAdjTriangleCount.Data[v];
		for (unsigned int poolIndex = entry; poolIndex < entry + count; ++poolIndex)
		{
			int triIndex = mAdjTriangleLists.Data[poolIndex];

			int v0 = mIndices.Data[triIndex * 3 + 0];
			int v1 = mIndices.Data[triIndex * 3 + 1];
			int v2 = mIndices.Data[triIndex * 3 + 2];

			if (v == v0)
			{
				triIndex = triIndex * 3 + 0;
			}
			else if (v == v1)
			{
				triIndex = triIndex * 3 + 1;
			}
			else if (v == v2)
			{
				triIndex = triIndex * 3 + 2;
			}
			else assert(false);
			mAdjTriangleLists.Data[poolIndex] = triIndex;
		}
	}
}


void Triangulator::SendDataToCPU(ID3D11DeviceContext* immediateContext)
{
	// get latest position from GPU
	mPositions.CopyToCpu(immediateContext);

	// read back result to CPU
	CopyTriangleColorsToCPU(immediateContext, true, true);
}

void Triangulator::IdentifyTrianglesToRefineThreshold()
{
	// assume that all data is copied to CPU
	// iterate the buffer and identify triangles to split
	mTrianglesToRefine.clear();

	const std::vector<Vec4u>& mean = mTriAccumColorSingle.Data;
	const std::vector<Vec4u>& variance = mTriVarianceColorSingle.Data;

	// do the refinement
	for (size_t tri = 0; tri < mTriVarianceColorSingle.Data.size(); ++tri)
	{
		Vec4u e = mTriVarianceColorSingle.Data[tri];
		Vec3f var = e.xyz().asfloat() / (255.f*255.f);
		float scalarVar = (var.x() + var.y() + var.z()) / 3.0f;
		scalarVar /= _Image->GetResolution().x() * _Image->GetResolution().y();
		scalarVar = (float)(std::sqrt(scalarVar) * 255.0);

		if (scalarVar > mRefineVarianceThreshold)
			mTrianglesToRefine.push_back((int)tri);
	}
}

void Triangulator::RefineMatlab()
{
	WritePositionsToMatlab("points_split");
	WriteIndicesToMatlab("triangles_split");
	WriteIntVectorToMatlab("idx_thr", mTrianglesToRefine);
	auto s1 = util::MatlabInterface::meval("[points_split, triangles_split, move_point_idx] = splitTriangles(points_split, triangles_split, idx_thr+1);");
	ReadPositionsFromMatlab("points_split");
	ReadIndicesFromMatlab("triangles_split");
	_CbParams.Data.NumVertices = (int)mPositions.Data.size();
}

void Triangulator::SendDataToGPU(ID3D11Device* device, ID3D11DeviceContext* immediateContext)
{
	// upload position and indices
	mPositions.CopyToGpu(device, immediateContext);
	mIndices.CopyToGpu(device, immediateContext);

	// resize the per triangle arrays
	mTriAccumColor.Data.resize(GetNumTriangles() * 13);
	mTriVarianceColor.Data.resize(GetNumTriangles() * 13);
	mTriAccumColorSingle.Data.resize(GetNumTriangles());
	mTriVarianceColorSingle.Data.resize(GetNumTriangles());
	mTriPlaneAccumCoeffs.Data.resize(GetNumTriangles() * 13);
	mTriPlaneEquations.Data.resize(GetNumTriangles() * 13);

	// resize the GPU buffers
	mTriAccumColor.CopyToGpu(device, immediateContext);

	mTriVarianceColor.CopyToGpu(device, immediateContext);
	mTriAccumColorSingle.CopyToGpu(device, immediateContext);
	mTriVarianceColorSingle.CopyToGpu(device, immediateContext);
	mTriPlaneAccumCoeffs.CopyToGpu(device, immediateContext);
	mTriPlaneEquations.CopyToGpu(device, immediateContext);

	// rebuild neighborhood
	BuildNeighborhoodInformation();
	BakeVertexIndexIntoAdjTriangleLists();

	// update the neighborhood information on the GPU
	mAdjTriangleLists.CopyToGpu(device, immediateContext);
	mAdjTriangleEntry.CopyToGpu(device, immediateContext);
	mAdjTriangleCount.CopyToGpu(device, immediateContext);
}

void Triangulator::MouseClick(bool leftDown, bool leftUp, bool rightDown, bool rightUp, int xPos, int yPos)
{
	if (_InteractionState == Interaction_Pick)
	{
		// in case the user does something
		if (leftDown || rightDown)
		{
			SendDataToCPU(_D3D->GetImmediateContext());
		}

		// compute the location that was clicked
		const D3D11_VIEWPORT fullViewport = _D3D->GetFullViewport();
		const BoundingBox2d& bbox = _Image->GetBoundingBox();
		Vec2f p((float)((xPos + 0.5) / fullViewport.Width * (bbox.GetMax().x() - bbox.GetMin().x())),
			(float)((1 - (yPos + 0.5) / fullViewport.Height) * (bbox.GetMax().y() - bbox.GetMin().y())));

		// pick the triangle
		Vec2f uv(10, 10);
		size_t index = PickTriangle(p, uv);
		if (index == 0xFFFFFFFF)
			return;

		// get indices of triangle
		int i0 = mIndices.Data[index + 0];
		int i1 = mIndices.Data[index + 1];
		int i2 = mIndices.Data[index + 2];

		// get the coordinates of the triangle
		Vec2f a = mPositions.Data[i0];
		Vec2f b = mPositions.Data[i1];
		Vec2f c = mPositions.Data[i2];

		//        c
		//       /\
		//      /  \
		//   B /    \ A
		//    /      \
		//   /        \
		// a ---------- b
		//        C

		// compute normals on each edge
		Vec2f nA = (b - c).normalize(); nA = Vec2f(-nA.y(), nA.x());
		Vec2f nB = (a - c).normalize(); nB = Vec2f(-nB.y(), nB.x());
		Vec2f nC = (a - b).normalize(); nC = Vec2f(-nC.y(), nC.x());

		// compute the distance to the edges
		float distA = dot(p - b, nA);	float dist2A = distA*distA;
		float distB = dot(p - a, nB);	float dist2B = distB*distB;
		float distC = dot(p - b, nC);	float dist2C = distC*distC;

		float pxsizeSqr = 15 * 15;
		bool closeToEdge = dist2A < pxsizeSqr || dist2B < pxsizeSqr || dist2C < pxsizeSqr;


		// add a vertex at the clicked location
		if (leftDown)
		{
			if (closeToEdge)
				AddVertexCPU_EdgeSplit(p, index);
			else
				AddVertexCPU_BaryCenter(p, index);
		}

		// remove the closest vertex
		if (rightDown)
		{
			if ((a - p).lengthSquared() < pxsizeSqr ||
				(b - p).lengthSquared() < pxsizeSqr ||
				(c - p).lengthSquared() < pxsizeSqr) {
				RemoveVertexCPU(p);
			}
			else if (closeToEdge) {
				EdgeFlipCPU(p, index);
			}
		}

		// in case the user did something -> upload the result to the GPU
		if (leftDown || rightDown)
		{
			SendDataToGPU(_D3D->GetDevice(), _D3D->GetImmediateContext());
		}
	}
	else if (_InteractionState == Interaction_Brush)
	{
		if (leftDown || rightDown)
			_IsBrushing = true;
		if (leftUp || rightUp)
			_IsBrushing = false;
		LARGE_INTEGER frequency, timerCurrent;
		QueryPerformanceCounter(&timerCurrent);
		QueryPerformanceFrequency(&frequency);
		mTimerBrushLast = (double)timerCurrent.QuadPart / (double)frequency.QuadPart;
	}
	else if (_InteractionState == Interaction_Select)
	{
		if (leftDown || rightDown)
			QueryPerformanceCounter(&timerLast);
	}
}

void Triangulator::MouseMove(bool leftDown, bool rightDown, int xPos, int yPos)
{
	mCursorPos = Vec2i(xPos, yPos);

	static std::default_random_engine rng;
	static std::uniform_real_distribution<float> rnd_uniform;
	static std::normal_distribution<float> rnd_normal;

	if (_InteractionState == Interaction_Brush && (leftDown || rightDown) && mBrushEventFrequency > 0)
	{
		// get elapsed time
		LARGE_INTEGER frequency, timerCurrent;
		QueryPerformanceCounter(&timerCurrent);
		QueryPerformanceFrequency(&frequency);
		double timeCurrent = (double)timerCurrent.QuadPart / (double)frequency.QuadPart;

		// can we do something?
		if (mTimerBrushLast + mBrushEventFrequency < timeCurrent)
		{
			// read data to CPU
			SendDataToCPU(_D3D->GetImmediateContext());

			// iteratively insert/delete
			while (mTimerBrushLast + mBrushEventFrequency < timeCurrent && mBrushEventFrequency > 0)
			{
				mTimerBrushLast += mBrushEventFrequency;

				// randomly sample a point in the disc
				Vec2f r(-10000, -10000);
				while (r.x()*r.x() + r.y()*r.y() > (float)(mBrushRadius*mBrushRadius))
					r = Vec2f(rnd_normal(rng), rnd_normal(rng)) * (float)mBrushRadius / 2.5f;
				
				// compute the location that was clicked
				const D3D11_VIEWPORT fullViewport = _D3D->GetFullViewport();
				const BoundingBox2d& bbox = _Image->GetBoundingBox();
				Vec2f p((float)((xPos + 0.5 + r.x()) / fullViewport.Width * (bbox.GetMax().x() - bbox.GetMin().x())),
					(float)((1 - (yPos + 0.5 + r.y()) / fullViewport.Height) * (bbox.GetMax().y() - bbox.GetMin().y())));

				// pick the triangle
				Vec2f uv(10, 10);
				size_t index = PickTriangle(p, uv);
				if (index == 0xFFFFFFFF)
					break;

				// get indices of triangle
				int i0 = mIndices.Data[index + 0];
				int i1 = mIndices.Data[index + 1];
				int i2 = mIndices.Data[index + 2];

				// get the coordinates of the triangle
				Vec2f a = mPositions.Data[i0];
				Vec2f b = mPositions.Data[i1];
				Vec2f c = mPositions.Data[i2];

				// compute normals on each edge
				Vec2f nA = (b - c).normalize(); nA = Vec2f(-nA.y(), nA.x());
				Vec2f nB = (a - c).normalize(); nB = Vec2f(-nB.y(), nB.x());
				Vec2f nC = (a - b).normalize(); nC = Vec2f(-nC.y(), nC.x());

				// compute the distance to the edges
				float distA = dot(p - b, nA);	float dist2A = distA*distA;
				float distB = dot(p - a, nB);	float dist2B = distB*distB;
				float distC = dot(p - b, nC);	float dist2C = distC*distC;

				// threshold for insertions
				float pxsizeSqr = 15 * 15;
				bool closeToEdge = dist2A < pxsizeSqr || dist2B < pxsizeSqr || dist2C < pxsizeSqr;
				//closeToEdge = true;

				// add a vertex at the clicked location
				if (leftDown)
				{
					if (closeToEdge)
						AddVertexCPU_EdgeSplit(p, index);
					else
						AddVertexCPU_BaryCenter(p, index);
				}

				if (rightDown)
				{
					int vid = -1;
					float vdist2 = std::numeric_limits<float>::max();
					for (size_t v = 0; v < mPositions.Data.size(); ++v)
					{
						float d = (p - mPositions.Data[v]).lengthSquared();
						if (d < vdist2) {
							vdist2 = d;
							vid = (int)v;
						}
					}

					if (!((mPositions.Data[vid].x() < 1 && mPositions.Data[vid].y() < 1) ||
						(mPositions.Data[vid].x() < 1 && mPositions.Data[vid].y() > _Image->GetResolution().y() - 1) ||
						(mPositions.Data[vid].x() > _Image->GetResolution().x() - 1 && mPositions.Data[vid].y() < 1) ||
						(mPositions.Data[vid].x() > _Image->GetResolution().x() - 1 && mPositions.Data[vid].y() > _Image->GetResolution().y() - 1))
						&& (mPositions.Data[vid]-c).lengthSquared() < mBrushRadius*mBrushRadius)
					{
						RemoveVertexCPU(p);
					}
					
				}

				// rebuild neighborhood
				BuildNeighborhoodInformation();
				BakeVertexIndexIntoAdjTriangleLists();
			}

			// upload data to GPU
			SendDataToGPU(_D3D->GetDevice(), _D3D->GetImmediateContext());
		}
	}
}

void Triangulator::MouseWheel(int zDelta)
{
	mBrushRadius = std::max(0, mBrushRadius + zDelta);
}

size_t Triangulator::PickTriangle(const Vec2f& p, Vec2f& uv)
{
	// check all triangles for click
	for (size_t index = 0; index < mIndices.Data.size(); index += 3)
	{
		// get indices of triangle
		int i0 = mIndices.Data[index + 0];
		int i1 = mIndices.Data[index + 1];
		int i2 = mIndices.Data[index + 2];

		// get get the coordinates of the triangle
		Vec2f a = mPositions.Data[i0];
		Vec2f b = mPositions.Data[i1];
		Vec2f c = mPositions.Data[i2];

		// compute bounding box of triangle
		Vec2f mini(std::min(std::min(a.x(), b.x()), c.x()), std::min(std::min(a.y(), b.y()), c.y()));
		Vec2f maxi(std::max(std::max(a.x(), b.x()), c.x()), std::max(std::max(a.y(), b.y()), c.y()));

		// do a bounding box check
		if (p.x() < mini.x() || maxi.x() < p.x() || p.y() < mini.y() || maxi.y() < p.y())
			continue;

		// compute the barycentric coordinate
		uv = Vec2f((b.x()*(p.y() - c.y()) + c.x()*(b.y() - p.y()) + (c.y() - b.y())*p.x()) / (a.x()*(c.y() - b.y()) + b.x()*(a.y() - c.y()) + (b.y() - a.y())*c.x()),
			-(a.x()*(p.y() - c.y()) + c.x()*(a.y() - p.y()) + (c.y() - a.y())*p.x()) / (a.x()*(c.y() - b.y()) + b.x()*(a.y() - c.y()) + (b.y() - a.y())*c.x()));

		// is inside the triangle?
		if (0 <= uv.x() && uv.x() <= 1 && 0 <= uv.y() && uv.y() <= 1 && uv.x() + uv.y() <= 1)
		{
			return index;
		}
	}
	return 0xFFFFFFFF;
}

void Triangulator::AddVertexCPU_BaryCenter(const Vec2f& p, size_t index)
{
	if (index == 0xFFFFFFFF) return;
	int i0 = mIndices.Data[index + 0];
	int i1 = mIndices.Data[index + 1];
	int i2 = mIndices.Data[index + 2];
	int inew = (int)mPositions.Data.size();
	mIndices.Data[index + 2] = inew;
	mIndices.Data.push_back(i1);
	mIndices.Data.push_back(i2);
	mIndices.Data.push_back(inew);
	mIndices.Data.push_back(i2);
	mIndices.Data.push_back(i0);
	mIndices.Data.push_back(inew);
	mPositions.Data.push_back(p);
}

static int orient2d(const Vec2f& A, const Vec2f& B, const Vec2f& p)
{
	float o = ((B.x() - A.x()) * (p.y() - A.y()) - (B.y() - A.y()) * (p.x() - A.x()));
	if (o < 0) return -1;
	else return 1;
}

void Triangulator::AddVertexCPU_EdgeSplit(const Vec2f& p, size_t index)
{
	// get indices of triangle
	int i0 = mIndices.Data[index + 0];
	int i1 = mIndices.Data[index + 1];
	int i2 = mIndices.Data[index + 2];

	// get the coordinates of the triangle
	Vec2f a = mPositions.Data[i0];
	Vec2f b = mPositions.Data[i1];
	Vec2f c = mPositions.Data[i2];

	//        c
	//       /\
	//      /  \
	//   B /    \ A
	//    /      \
	//   /        \
	// a ---------- b
	//        C

	// compute normals on each edge
	Vec2f vA = (b - c).normalize(); Vec2f nA(-vA.y(), vA.x());
	Vec2f vB = (a - c).normalize(); Vec2f nB(-vB.y(), vB.x());
	Vec2f vC = (a - b).normalize(); Vec2f nC(-vC.y(), vC.x());

	// compute the distance to the edges
	float distA = dot(p - b, nA);	float dist2A = distA*distA;
	float distB = dot(p - a, nB);	float dist2B = distB*distB;
	float distC = dot(p - b, nC);	float dist2C = distC*distC;

	// get the vertex indices of the closest edge and the closest point on the edge
	int e0 = -1, e1 = -1;
	Vec2f newpos = p;
	if (dist2A <= dist2B && dist2A <= dist2C) {
		e0 = i1; e1 = i2;
		newpos = c + vA * dot(vA, p - c);
	}
	if (dist2B <= dist2A && dist2B <= dist2C) {
		e0 = i0; e1 = i2;
		newpos = c + vB * dot(vB, p - c);
	}
	if (dist2C <= dist2A && dist2C <= dist2B) {
		e0 = i0; e1 = i1;
		newpos = b + vC * dot(vC, p - b);
	}

	// iterate the one-ring of one of the two and search for two faces that will swap edges
	std::list<int> trianglesToSwap;
	int ventry = mAdjTriangleEntry.Data[e0];
	int vcount = mAdjTriangleCount.Data[e0];
	for (int neighborListId = ventry; neighborListId < ventry + vcount; ++neighborListId)
	{
		// iterate the vertices of the neighbor triangle
		int neighborTri = mAdjTriangleLists.Data[neighborListId] / 3;	// <- unbake
		int vcorner = -1;
		int ncorner = -1;

		for (int ioff = 0; ioff < 3; ++ioff) {
			int i0 = mIndices.Data[neighborTri * 3 + ioff];
			if (i0 == e0) vcorner = ioff;
			if (i0 == e1) ncorner = ioff;
		}

		// triangle will survive
		if (ncorner != -1 && vcorner != -1) {
			trianglesToSwap.push_back(neighborTri);
		}
	}
	if (trianglesToSwap.size() != 2) return;

	// get the remaining vertex of the left triangle
	int eL = -1;
	for (int ioff = 0; ioff < 3; ++ioff) {
		int ic = mIndices.Data[trianglesToSwap.front() * 3 + ioff];
		if (ic != e0 && ic != e1) {
			eL = ic;
			break;
		}
	}
	
	// get the remaining vertex of the right triangle
	int eR = -1;
	for (int ioff = 0; ioff < 3; ++ioff) {
		int ic = mIndices.Data[trianglesToSwap.back() * 3 + ioff];
		if (ic != e0 && ic != e1) {
			eR = ic;
			break;
		}
	}

	int nl = trianglesToSwap.front();
	int nr = trianglesToSwap.back();

	int inew = (int)mPositions.Data.size();
	mPositions.Data.push_back(newpos);

	mIndices.Data[nl * 3 + 0] = inew;
	mIndices.Data[nl * 3 + 1] = eL;
	mIndices.Data[nl * 3 + 2] = e0;
	
	mIndices.Data[nr * 3 + 0] = inew;
	mIndices.Data[nr * 3 + 1] = e1;
	mIndices.Data[nr * 3 + 2] = eL;
	
	mIndices.Data.push_back(inew);
	mIndices.Data.push_back(eR);
	mIndices.Data.push_back(e0);

	mIndices.Data.push_back(inew);
	mIndices.Data.push_back(eR);
	mIndices.Data.push_back(e1);		
}

void Triangulator::RemoveVertexCPU(const Vec2f& p)
{
	// iterate all vertices and find the closest one to the clicked location
	int vid = -1;
	float vdist2 = std::numeric_limits<float>::max();
	for (size_t v = 0; v < mPositions.Data.size(); ++v)
	{
		float d = (p - mPositions.Data[v]).lengthSquared();
		if (d < vdist2) {
			vdist2 = d;
			vid = (int)v;
		}
	}

	if ((mPositions.Data[vid].x() < 1 && mPositions.Data[vid].y() < 1) ||
		(mPositions.Data[vid].x() < 1 && mPositions.Data[vid].y() > _Image->GetResolution().y() - 1) ||
		(mPositions.Data[vid].x() > _Image->GetResolution().x() - 1 && mPositions.Data[vid].y() < 1) ||
		(mPositions.Data[vid].x() > _Image->GetResolution().x() - 1 && mPositions.Data[vid].y() > _Image->GetResolution().y() - 1))
		return;

	// iterate the 1-ring and find the closest neighbor -> we will collapse to this point
	int ventry = mAdjTriangleEntry.Data[vid];
	int vcount = mAdjTriangleCount.Data[vid];
	int nid = -1;
	float closestNeighbor = std::numeric_limits<float>::max();
	for (int neighborListId = ventry; neighborListId < ventry + vcount; ++neighborListId)
	{
		// iterate the vertices of the neighbor triangle
		int neighborTri = mAdjTriangleLists.Data[neighborListId] / 3;	// <- unbake
		for (int ioff = 0; ioff < 3; ++ioff)
		{
			int i0 = mIndices.Data[neighborTri * 3 + ioff];
			if (i0 != vid)	// if not vertex vid
			{
				// compute closest neighbor
				float dist = (mPositions.Data[i0] - mPositions.Data[vid]).lengthSquared();
				if (dist < closestNeighbor)
				{
					closestNeighbor = dist;
					nid = i0;
				}
			}
		}
	}

	// iterate neighbor triangles again and adjust the index buffers
	std::list<int> trianglesToDelete;
	for (int neighborListId = ventry; neighborListId < ventry + vcount; ++neighborListId)
	{
		// iterate the vertices of the neighbor triangle
		int neighborTri = mAdjTriangleLists.Data[neighborListId] / 3;	// <- unbake
		int vcorner = -1;
		int ncorner = -1;

		for (int ioff = 0; ioff < 3; ++ioff) {
			int i0 = mIndices.Data[neighborTri * 3 + ioff];
			if (i0 == vid) vcorner = ioff;
			if (i0 == nid) ncorner = ioff;
		}

		// triangle will survive
		if (ncorner == -1) {
			mIndices.Data[neighborTri * 3 + vcorner] = nid;
		}
		else {	// triangle will collapse
			trianglesToDelete.push_back(neighborTri);
		}
	}

	// delete the two triangles from the index buffer
	{
		trianglesToDelete.sort();
		while (!trianglesToDelete.empty())
		{
			for (int i = trianglesToDelete.back(); i < GetNumTriangles() - 1; ++i)
			{
				mIndices.Data[i * 3 + 0] = mIndices.Data[(i + 1) * 3 + 0];
				mIndices.Data[i * 3 + 1] = mIndices.Data[(i + 1) * 3 + 1];
				mIndices.Data[i * 3 + 2] = mIndices.Data[(i + 1) * 3 + 2];
			}
			mIndices.Data.resize(mIndices.Data.size() - 3);
			trianglesToDelete.pop_back();
		}
	}

	// delete the vertex from the vertex buffer
	for (size_t v = vid; v < mPositions.Data.size() - 1; ++v)
		mPositions.Data[v] = mPositions.Data[v + 1];
	mPositions.Data.resize(mPositions.Data.size() - 1);

	for (size_t i = 0; i < mIndices.Data.size(); ++i)
	{
		if (mIndices.Data[i] > (unsigned int)vid)
			mIndices.Data[i]--;
	}
}

void Triangulator::EdgeFlipCPU(const Vec2f& p, size_t index)
{
	// get indices of triangle
	int i0 = mIndices.Data[index + 0];
	int i1 = mIndices.Data[index + 1];
	int i2 = mIndices.Data[index + 2];

	// get the coordinates of the triangle
	Vec2f a = mPositions.Data[i0];
	Vec2f b = mPositions.Data[i1];
	Vec2f c = mPositions.Data[i2];

	//        c
	//       /\
	//      /  \
	//   B /    \ A
	//    /      \
	//   /        \
	// a ---------- b
	//        C

	// compute normals on each edge
	Vec2f nA = (b - c).normalize(); nA = Vec2f(-nA.y(), nA.x());
	Vec2f nB = (a - c).normalize(); nB = Vec2f(-nB.y(), nB.x());
	Vec2f nC = (a - b).normalize(); nC = Vec2f(-nC.y(), nC.x());

	// compute the distance to the edges
	float distA = dot(p - b, nA);	float dist2A = distA*distA;
	float distB = dot(p - a, nB);	float dist2B = distB*distB;
	float distC = dot(p - b, nC);	float dist2C = distC*distC;

	// get the vertex indices of the closest edge
	int e0 = -1, e1 = -1;
	if (dist2A <= dist2B && dist2A <= dist2C) {
		e0 = i1; e1 = i2;
	}
	if (dist2B <= dist2A && dist2B <= dist2C) {
		e0 = i0; e1 = i2;
	}
	if (dist2C <= dist2A && dist2C <= dist2B) {
		e0 = i0; e1 = i1;
	}

	// iterate the one-ring of one of the two and search for two faces that will swap edges
	std::list<int> trianglesToSwap;
	int ventry = mAdjTriangleEntry.Data[e0];
	int vcount = mAdjTriangleCount.Data[e0];
	for (int neighborListId = ventry; neighborListId < ventry + vcount; ++neighborListId)
	{
		// iterate the vertices of the neighbor triangle
		int neighborTri = mAdjTriangleLists.Data[neighborListId] / 3;	// <- unbake
		int vcorner = -1;
		int ncorner = -1;

		for (int ioff = 0; ioff < 3; ++ioff) {
			int i0 = mIndices.Data[neighborTri * 3 + ioff];
			if (i0 == e0) vcorner = ioff;
			if (i0 == e1) ncorner = ioff;
		}

		// triangle will survive
		if (ncorner != -1 && vcorner != -1) {
			trianglesToSwap.push_back(neighborTri);
		}
	}
	if (trianglesToSwap.size() != 2) return;

	// get the remaining vertex of the left triangle
	int el = -1;
	for (int ioff = 0; ioff < 3; ++ioff) {
		int ic = mIndices.Data[trianglesToSwap.front() * 3 + ioff];
		if (ic != e0 && ic != e1) {
			el = ic;
			break;
		}
	}

	// get the remaining vertex of the right triangle
	int er = -1;
	for (int ioff = 0; ioff < 3; ++ioff) {
		int ic = mIndices.Data[trianglesToSwap.back() * 3 + ioff];
		if (ic != e0 && ic != e1) {
			er = ic;
			break;
		}
	}

	// are the four vertices convex?
	if ((orient2d(mPositions.Data[el], mPositions.Data[er], mPositions.Data[e0]) !=
		orient2d(mPositions.Data[el], mPositions.Data[er], mPositions.Data[e1]))
		&&
		(orient2d(mPositions.Data[e0], mPositions.Data[e1], mPositions.Data[el]) !=
			orient2d(mPositions.Data[e0], mPositions.Data[e1], mPositions.Data[er])))
	{
		int nl = trianglesToSwap.front();
		int nr = trianglesToSwap.back();

		mIndices.Data[nl * 3 + 0] = el;
		mIndices.Data[nl * 3 + 1] = e0;
		mIndices.Data[nl * 3 + 2] = er;

		mIndices.Data[nr * 3 + 0] = el;
		mIndices.Data[nr * 3 + 1] = er;
		mIndices.Data[nr * 3 + 2] = e1;
	}
}

static bool Circumcircle(const Vec2d& p0, const Vec2d& p1, const Vec2d& p2, Vec2d& center, double& radius) {
	double dA, dB, dC, aux1, aux2, div;

	dA = p0.x() * p0.x() + p0.y() * p0.y();
	dB = p1.x() * p1.x() + p1.y() * p1.y();
	dC = p2.x() * p2.x() + p2.y() * p2.y();

	aux1 = (dA*(p2.y() - p1.y()) + dB*(p0.y() - p2.y()) + dC*(p1.y() - p0.y()));
	aux2 = -(dA*(p2.x() - p1.x()) + dB*(p0.x() - p2.x()) + dC*(p1.x() - p0.x()));
	div = (2 * (p0.x()*(p2.y() - p1.y()) + p1.x()*(p0.y() - p2.y()) + p2.x()*(p1.y() - p0.y())));

	if (div == 0) {
		return false;
	}

	center.x() = aux1 / div;
	center.y() = aux2 / div;

	radius = sqrt((center.x() - p0.x())*(center.x() - p0.x()) + (center.y() - p0.y())*(center.y() - p0.y()));
	return true;
}

static float inangle(const Vec2f& a, const Vec2f& b0, const Vec2f& b1)
{
	return acos(dot((b0 - a).normalized(), (b1 - a).normalized()));
}

void Triangulator::DelaunayEdelsbrunner()
{
#if 0
	const Vec2f& p = mPositions.Data[inew];

	// iterate all triangles and find the ones that contain p
	std::vector<int> badTriangleOffsets;
	for (int index = 0; index < GetNumTriangles(); ++index)
	{
		// get indices of triangle
		int i0 = mIndices.Data[index*3 + 0];
		int i1 = mIndices.Data[index*3 + 1];
		int i2 = mIndices.Data[index*3 + 2];

		// get the coordinates of the triangle
		Vec2f a = mPositions.Data[i0];
		Vec2f b = mPositions.Data[i1];
		Vec2f c = mPositions.Data[i2];

		// compute circum circle
		Vec2d d; double r;
		if (Circumcircle(a.asdouble(), b.asdouble(), c.asdouble(), d, r))
		{
			// is the point contained?
			if ((d - p.asdouble()).lengthSquared() <= (r*r) + 1E-6)
			{
				// remember the triangle!
				badTriangleOffsets.push_back(index);
			}
		}
	}

	// find the outer edges of the bad triangle region
	std::set<std::pair<int, int>> edges;
	for (int index : badTriangleOffsets)
	{
		// get indices of triangle
		int i0 = mIndices.Data[index * 3 + 0];
		int i1 = mIndices.Data[index * 3 + 1];
		int i2 = mIndices.Data[index * 3 + 2];

		if (inew != i0 && inew != i1){
			std::pair<int, int> e(i0, i1);
			if (e.first < e.second)	std::swap(e.first, e.second);
			if (edges.find(e) == edges.end())
				edges.insert(e);
			else edges.erase(e);
		}
		if (inew != i0 && inew != i2) {
			std::pair<int, int> e(i0, i2);
			if (e.first < e.second)	std::swap(e.first, e.second);
			if (edges.find(e) == edges.end())
				edges.insert(e);
			else edges.erase(e);
		}
		if (inew != i1 && inew != i2) {
			std::pair<int, int> e(i1, i2);
			if (e.first < e.second)	std::swap(e.first, e.second);
			if (edges.find(e) == edges.end())
				edges.insert(e);
			else edges.erase(e);
		}
	}

	// check numerical errors (unfortunately not unlikely)
	//assert(edges.size() == badTriangleOffsets.size());
	auto e = edges.begin();
	for (size_t i = 0; i < std::min(edges.size(), badTriangleOffsets.size()); ++i)
	{
		int index = badTriangleOffsets[i];
		mIndices.Data[index * 3 + 0] = e->first;
		mIndices.Data[index * 3 + 1] = e->second;
		mIndices.Data[index * 3 + 2] = inew;
		e++;
	}

#else

	bool changed;
	do
	{
		changed = false;

		for (int tri = 0; tri < GetNumTriangles(); ++tri)
		{
			// get indices of triangle
			int i0 = mIndices.Data[tri * 3 + 0];
			int i1 = mIndices.Data[tri * 3 + 1];
			int i2 = mIndices.Data[tri * 3 + 2];
			
			std::vector<std::pair<int, int>> triedges;
			triedges.push_back(std::pair<int, int>(i0, i1));
			triedges.push_back(std::pair<int, int>(i0, i2));
			triedges.push_back(std::pair<int, int>(i1, i2));

			for (auto& edge : triedges)
			{
				int e0 = edge.first;
				int e1 = edge.second;

				// iterate the one-ring of one of the two and search for two faces that will swap edges
				std::list<int> trianglesToSwap;
				int ventry = mAdjTriangleEntry.Data[e0];
				int vcount = mAdjTriangleCount.Data[e0];
				for (int neighborListId = ventry; neighborListId < ventry + vcount; ++neighborListId)
				{
					// iterate the vertices of the neighbor triangle
					int neighborTri = mAdjTriangleLists.Data[neighborListId] / 3;	// <- unbake
					int vcorner = -1;
					int ncorner = -1;

					for (int ioff = 0; ioff < 3; ++ioff) {
						int i0 = mIndices.Data[neighborTri * 3 + ioff];
						if (i0 == e0) vcorner = ioff;
						if (i0 == e1) ncorner = ioff;
					}

					// triangle will survive
					if (ncorner != -1 && vcorner != -1) {
						trianglesToSwap.push_back(neighborTri);
					}
				}
				if (trianglesToSwap.size() != 2) 
					continue;

				// get the remaining vertex of the left triangle
				int el = -1;
				for (int ioff = 0; ioff < 3; ++ioff) {
					int ic = mIndices.Data[trianglesToSwap.front() * 3 + ioff];
					if (ic != e0 && ic != e1) {
						el = ic;
						break;
					}
				}

				// get the remaining vertex of the right triangle
				int er = -1;
				for (int ioff = 0; ioff < 3; ++ioff) {
					int ic = mIndices.Data[trianglesToSwap.back() * 3 + ioff];
					if (ic != e0 && ic != e1) {
						er = ic;
						break;
					}
				}

				float angle1 = inangle(mPositions.Data[el], mPositions.Data[e0], mPositions.Data[e1]);
				float angle2 = inangle(mPositions.Data[er], mPositions.Data[e0], mPositions.Data[e1]);

				float beta1 = inangle(mPositions.Data[e0], mPositions.Data[el], mPositions.Data[er]);
				float beta2 = inangle(mPositions.Data[e1], mPositions.Data[el], mPositions.Data[er]);

				float alpha = angle1 + angle2;
				float beta = beta1 + beta2;
				float all = alpha + beta;

				
				if (M_PI + 1E-6 < angle1 + angle2)
				{
					int nl = trianglesToSwap.front();
					int nr = trianglesToSwap.back();

					mIndices.Data[nl * 3 + 0] = el;
					mIndices.Data[nl * 3 + 1] = e0;
					mIndices.Data[nl * 3 + 2] = er;

					mIndices.Data[nr * 3 + 0] = el;
					mIndices.Data[nr * 3 + 1] = er;
					mIndices.Data[nr * 3 + 2] = e1;

					changed = true;
					break;
				}
				if (changed) 
					break;
			}
			if (changed)
				break;
		}
		BuildNeighborhoodInformation();
		BakeVertexIndexIntoAdjTriangleLists();
	} while (changed);

#endif
	
}
