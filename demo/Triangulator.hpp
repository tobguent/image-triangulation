#pragma once

class UI;
class UIBar;
class Camera2D;
class Image2DView;
class ImmediateRenderer2D;
class RenderTarget2D;
class Texture2D;

#include "D3D.h"

// union that represents either a 32bit float or a 32bit unsigned int
union FU {
	float Float;
	unsigned int Uint;
};

// define vectors containing four, nine, or fifteen elements. those will be used to represent the information that is accumulated over the triangles (average colors or system matrix coefficients)
struct Vec4uu { FU Data[4]; };
struct Vec9u { FU Data[9]; };
struct Vec15u { FU Data[15]; };

// implementation of "Stylized image triangulation", Computer Graphics Forum 2019.
class Triangulator
{
	
public:
	// enumeration of the visualizations that can be shown
	enum EDrawMode
	{
		DrawMode_Wireframe,
		DrawMode_Color,
		DrawMode_Variance,
		DrawMode_Tex,
	};

	// enumeration of the coloring options
	enum EFillMode
	{
		FillMode_Constant,
		FillMode_Linear,
	};

	// enumeration of the manual interaction options
	enum EInteractionState
	{
		Interaction_Pick,
		Interaction_Brush,
		Interaction_Select,
	};

	// constant buffer with parameters for the compute shaders
	struct CbParams
	{
		int NumVertices;		// number of vertices
		float StepSize;			// step size in gradient descent
		Vec2f DomainSize;		// size of the domain
		float UmbrellaDamping;	// weight of the regularization term
		float TrustRegion;		// the gradient descent bounds the steps per iteration by this distance
	};

	// constructor
	Triangulator(Image2DView* image, float refineVarianceThreshold);
	// destructor
	~Triangulator();

	// initializes the user interface
	void InitUI(UI* ui, UIBar* bar);

	// Creates the resource and its views.
	bool D3DCreateDevice(ID3D11Device* Device);
	// Releases the resource and its views.
	void D3DReleaseDevice();

	// Create resources that depend on the screen resolution.
	bool D3DCreateSwapChain(ID3D11Device* Device, IDXGISwapChain* SwapChain, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc);
	// Release resources that depend on the screen resolution.
	void D3DReleaseSwapChain();

	// Passing down user interactions
	void MouseClick(bool leftDown, bool leftUp, bool rightDown, bool rightUp, int xPos, int yPos);
	void MouseMove(bool leftDown, bool rightDown, int xPos, int yPos);
	void MouseWheel(int zDelta);

	// Renders the triangulation with its given options
	void Draw(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D);

	// Performs one iteration on the optimization and refinement
	void Iterate(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* d3d);

	// Gets the current number of triangles
	int GetNumTriangles() const { return (int)mIndices.Data.size() / 3; }
	// Gets the image viewer
	Image2DView* GetImageView() { return _Image; }

	// Copies triangle information back to the CPU. This is information used for the CPU-based refinement
	void CopyTriangleColorsToCPU(ID3D11DeviceContext* immediateContext, bool accumulatedColor = true, bool varianceColor = true);

	// Removes the current selection
	void ClearSelection();
	// Applies a Delaunay triangulation in the selected area.
	void DelaunaySelection();

private:

	// Draw the currently selected visualization mode
	void DrawMode(EDrawMode drawMode, ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D);
	// When in selection mode, the user can draw a mask for selections. This call adds to the mask at the current cursor position.
	void DrawAddToMask(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D);
	// Renders the current selection mask as user feedback.
	void DrawMaskContour(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D);

	// Builds a regular grid triangulation with Matab
	void ReadTriangulationFromMatlab(const Vec2i& res);
	
	void CopyTriangleColorsToSingle(ID3D11ShaderResourceView* srvToCopy, ID3D11UnorderedAccessView* uavTarget, ID3D11DeviceContext* immediateContext);
	void CopyPositionsToCPU(ID3D11DeviceContext* immediateContext);
	void BuildNeighborhoodInformation();
	void BakeVertexIndexIntoAdjTriangleLists();
	void UpdatePositions(ID3D11DeviceContext* immediateContext);

	
	void SendDataToCPU(ID3D11DeviceContext* immediateContext);
	void IdentifyTrianglesToRefineThreshold();
	void RefineMatlab();
	void SendDataToGPU(ID3D11Device* device, ID3D11DeviceContext* immediateContext);

	void ReadPositionsFromMatlab(const std::string& positionVar);
	void ReadIndicesFromMatlab(const std::string& indexVar);

	void WritePositionsToMatlab(const std::string& positionVar);
	void WriteIndicesToMatlab(const std::string& indexVar);
	void WriteIntVectorToMatlab(const std::string& vectorVar, const std::vector<int>& vec);

	void ComputeConstVariance(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D);	// computes the error residual for the constant colors
	void ComputePlaneVariance(ID3D11DeviceContext* ImmediateContext, Camera2D* camera, D3D* D3D);	// computes the error residual for the linear gradients

	size_t PickTriangle(const Vec2f& p, Vec2f& uv);
	void AddVertexCPU_BaryCenter(const Vec2f& p, size_t index);	// inserts a vertex at barycenter
	void AddVertexCPU_EdgeSplit(const Vec2f& p, size_t index);	// splits an edge at the selected point
	void RemoveVertexCPU(const Vec2f& p);						// picks closest vertex and does an edge collapse
	void EdgeFlipCPU(const Vec2f& p, size_t index);				// flips the edge that closest to the selected point
	void DelaunayEdelsbrunner();								// call after insertion: iteratively regains a Delaunay triangulation

	float mRefineVarianceThreshold;
	bool mEnableOptimization;
	int mRefineNthIteration;
	int mDelaunayNthIteration;

	DynamicByteAddressBuffer<Vec2f> mPositions;					// vertex buffer
	DynamicByteAddressBuffer<unsigned int> mIndices;			// index buffer (triangle list)
	DynamicByteAddressBuffer<unsigned int> mAdjTriangleLists;	// list of all the neighbors
	DynamicByteAddressBuffer<unsigned int> mAdjTriangleEntry;	// entry into the list of neighboring triangles
	DynamicByteAddressBuffer<unsigned int> mAdjTriangleCount;	// number of entries neighbor list
	DynamicByteAddressBuffer<Vec4uu> mTriAccumColor;			// RGB, count
	DynamicByteAddressBuffer<Vec4uu> mTriVarianceColor;			// variance per channel
	DynamicByteAddressBuffer<Vec4u> mTriAccumColorSingle;		// RGB, count
	DynamicByteAddressBuffer<Vec4u> mTriVarianceColorSingle;	// variance per channel
	
	DynamicByteAddressBuffer<Vec15u> mTriPlaneAccumCoeffs;
	DynamicByteAddressBuffer<Vec9u> mTriPlaneEquations;

	std::vector<int> mTrianglesToRefine;

	ID3DX11Effect* _FxRasterVariance;
	ID3DX11Effect* _FxDisplayTriangulation;

	ID3DX11Effect* _FxRasterPlane;
	ID3D11ComputeShader* _CsSolvePlane;

	ID3DX11Effect* _FxBrushing;

	ID3D11InputLayout* _ILTriangles;

	ID3D11ComputeShader* _CsCopyToStaging;		// copies one layer into our single layer buffer
	ID3D11ComputeShader* _CsUpdatePosition;		// performs a gradient descent step
	ID3D11ComputeShader* _CsSaturateTexture;	// clamps the values in the mask texture

	ConstantBuffer<CbParams> _CbParams;			// stores the parameters for the compute shaders

	EDrawMode _DrawMode;	// the currently selected visualization mode
	float _StepSize;		// step size of the gradient descent
	Vec2f _VarianceBounds;	// min and max value range for the color-coding of the approximation residual

	EFillMode _FillMode;
	EInteractionState _InteractionState;
	bool _IsBrushing;
	bool _ShowWireframe;
	Vec3f mWireframeColor;
	int _RefineIterations;

	Image2DView* _Image;

	ImmediateRenderer2D* mImmediateRenderer2D;
	Vec2i mCursorPos;
	int mBrushRadius;
	float mBrushEventFrequency;
	double mTimerBrushLast;
	
	RenderTarget2D* mMask;
	int mMaskDelaunayNumPoints;

};