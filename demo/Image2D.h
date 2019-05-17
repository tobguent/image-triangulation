#pragma

#include "math.hpp"

class Image2D
{
public:

	// Constructor. Reserves the memory for the frame.
	Image2D(const Vec2i& screenResolution) : _ScreenResolution(screenResolution) {
		_Color.resize(screenResolution.x()*screenResolution.y(), Vec3d(0, 0, 0));
	}

	// Constructor. Reads an image from file
	Image2D(const char* filename) : _ScreenResolution(0,0) {
		ImportBmp(filename);
	}

	// Gets the pixel data at a certain coordinate
	Vec3d& GetPixel(const Vec2i& pxCoord) { return _Color[pxCoord.y() * _ScreenResolution.x() + pxCoord.x()]; }
	// Gets the pixel data at a certain coordinate
	const Vec3d& GetPixel(const Vec2i& pxCoord) const { return _Color[pxCoord.y() * _ScreenResolution.x() + pxCoord.x()]; }
	// Get the resolution of the viewport
	const Vec2i& GetResolution() const { return _ScreenResolution; }		

	// Exports the current frame to a pfm file.
	void ExportBmp(const char* filename) const;
	// imports an image
	bool ImportBmp(const char* filename);

private:

	std::vector<Vec3d> _Color;
	Vec2i _ScreenResolution;		// resolution of the viewport
};

class Camera2D;

// Helper used to draw an image2D.
class Image2DView
{
public:
	// Constructor
	explicit Image2DView(const Vec2i& resolution, const BoundingBox2d& box);

	// Destructor.
	~Image2DView();

	// Creates the resource and its views.
	bool D3DCreateDevice(ID3D11Device* Device);

	// Releases the resource and its views.
	void D3DReleaseDevice();

	// Submits the draw call.
	void DrawToScreen(ID3D11DeviceContext* ImmediateContext);
	void DrawToScreen(ID3D11DeviceContext* ImmediateContext, ID3D11ShaderResourceView* otherSrv, bool singleComponent = false);
	void DrawToDomain(ID3D11DeviceContext* ImmediateContext, Camera2D* camera);

	void SetImage(Image2D* image, ID3D11DeviceContext* ImmediateContext);

	const Vec2i& GetResolution() const { return _Resolution; }

	ID3D11ShaderResourceView* GetSrv() { return _Srv; }
	const BoundingBox2d& GetBoundingBox() const { return _BoundingBox; }

private:

	void UpdateGPUResources(ID3D11DeviceContext* ImmediateContext);

	// Effect that displays the film.
	ID3DX11Effect* _Effect;

	ID3D11Texture2D* _Tex;
	ID3D11ShaderResourceView* _Srv;

	const Vec2i _Resolution;
	BoundingBox2d _BoundingBox;

	bool _MarkedDirty;

	// The film to display
	std::vector<Vec4f> _Data;
};