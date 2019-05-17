#include "stdafx.h"
#include "D3D.h"
#include "UI.h"
#include "ImmediateRenderer2D.h"
#include "Image2D.h"
#include "Triangulator.hpp"
#include <windowsx.h>

// ---------------------------------------
// Global variables
// ---------------------------------------
D3D*			_D3D;						// Comprises context and backbuffer creation
UI*				_UI;						// Manages all UI elements
UIBar*			_UIBar;						// The UI window that contains the sliders etc.
XMFLOAT4		_ClearColor(1,1,1,1);		// Clear color of the back buffer
Camera2D*		_Camera2D = NULL;
Image2DView*	_ImageViewer = NULL;
Triangulator*	_Triangulator = NULL;
ImmediateRenderer2D* _ImmediateRenderer2D = new ImmediateRenderer2D();
std::string g_ResourcePath = "";

// ---------------------------------------
// Window-related code
// ---------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HDC hdc;

	bool userInteracts = false;
	if (!_UI || _UI->IsVisible())
		_UI->MessageProc(hWnd, message, wParam, lParam);
	if (_UI && _UI->IsVisible()) {
		userInteracts = _UI->IsMouseOver() || _UI->IsUserInteracting();
	}

	int xPos = 0, yPos = 0, fwKeys = 0, zDelta = 0;

	switch( message )
	{
		case WM_PAINT:
			hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		case WM_LBUTTONDOWN:
			if (!userInteracts) {
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
				_Triangulator->MouseClick(true, false, false, false, xPos, yPos);
			}
			break;
		case WM_LBUTTONUP:
			if (!userInteracts) {
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
				_Triangulator->MouseClick(false, true, false, false, xPos, yPos);
			}
			break;
		case WM_RBUTTONDOWN:
			if (!userInteracts) {
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
				_Triangulator->MouseClick(false, false, true, false, xPos, yPos);
			}
			break;
		case WM_RBUTTONUP:
			if (!userInteracts) {
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
				_Triangulator->MouseClick(false, false, false, true, xPos, yPos);
			}
			break;

		case WM_MOUSEMOVE:
		{
			if (!userInteracts) {
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
				fwKeys = GET_KEYSTATE_WPARAM(wParam);
				_Triangulator->MouseMove(
					(fwKeys & MK_LBUTTON) == MK_LBUTTON,
					(fwKeys & MK_RBUTTON) == MK_RBUTTON,
					xPos, yPos);
			}
			break;
		}

		case WM_MOUSEWHEEL:
		{
			if (!userInteracts) {
				zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				_Triangulator->MouseWheel(zDelta);
			}
			break;
		}

		case WM_GETMINMAXINFO:
		{
			DefWindowProc(hWnd, message, wParam, lParam);
			MINMAXINFO* pmmi = (MINMAXINFO*)lParam;
			pmmi->ptMaxTrackSize.x = 10000;
			pmmi->ptMaxTrackSize.y = 10000;
			break;
		}

		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
	}

	return 0;
}

HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow, const std::string& WindowTitle, LONG Width, LONG Height, HWND& hWnd)
{
	// Register class
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof( WNDCLASSEX );
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = "ImageTriClass";
	if( !RegisterClassEx( &wcex ) )
		return E_FAIL;

	// Create window
	RECT rc = { 0, 0, Width, Height };	
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	hWnd = CreateWindow( "ImageTriClass", WindowTitle.c_str(), WS_OVERLAPPEDWINDOW,
						   CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
						   NULL );
	if( !hWnd )
		return E_FAIL;

	ShowWindow( hWnd, nCmdShow );

	return S_OK;
}

// ---------------------------------------
// Resource callbacks
// ---------------------------------------

// Creates resources independent of the viewport resolution
bool D3DCreateDevice(ID3D11Device* Device, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc)
{
	if (!_ImmediateRenderer2D->D3DCreateDevice(Device)) return false;
	if (_ImageViewer && !_ImageViewer->D3DCreateDevice(Device)) return false;
	if (_Triangulator && !_Triangulator->D3DCreateDevice(Device)) return false;
	return true;
}
// Creates resources dependent on the viewport resolution
bool D3DCreateSwapChain(ID3D11Device* Device, IDXGISwapChain* SwapChain, const DXGI_SURFACE_DESC* BackBufferSurfaceDesc)
{
	if (_Triangulator && !_Triangulator->D3DCreateSwapChain(Device, SwapChain, BackBufferSurfaceDesc)) return false;
	return true;
}
// Destroys resources independent of the viewport resolution
void D3DReleaseDevice()
{
	if (_ImmediateRenderer2D) _ImmediateRenderer2D->D3DReleaseDevice();
	if (_ImageViewer) _ImageViewer->D3DReleaseDevice();
	if (_Triangulator) _Triangulator->D3DReleaseDevice();
}
// Destroys resources dependent on the viewport resolution
void D3DReleaseSwapChain()
{
	if (_Triangulator) _Triangulator->D3DReleaseSwapChain();
}

// ---------------------------------------
// Update and Render
// ---------------------------------------
void Render()
{
	{
		// Collect objects needed later
		ID3D11DeviceContext* immediateContext = _D3D->GetImmediateContext();
		ID3D11RenderTargetView* rtvBackbuffer = _D3D->GetRtvBackbuffer();
		ID3D11DepthStencilView* dsvBackbuffer = _D3D->GetDsvBackbuffer();

		// Clear the backbuffer and the depth buffer
		immediateContext->ClearRenderTargetView(rtvBackbuffer, &_ClearColor.x);
		immediateContext->ClearDepthStencilView(dsvBackbuffer, D3D11_CLEAR_DEPTH, 1, 0);

		immediateContext->OMSetDepthStencilState(DepthStencilStates::DsTestWriteOff(), 0);

		if (_Triangulator)
		{
			_Triangulator->Iterate(immediateContext, _Camera2D, _D3D);
			_Triangulator->Draw(immediateContext, _Camera2D, _D3D);
		}
	}
	
	// Draw the UI and swap
	_UI->RenderUI();
	_D3D->GetSwapChain()->Present(0,0);
}

static void UI_CALL ExportBackbuffer(void *clientData) {
	((D3D*)clientData)->ExportBackbufferToFile();
}

// ---------------------------------------
// Entry point
// ---------------------------------------
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// Allocate a debug console and redirect standard streams
	AllocConsole();
	FILE *pCout, *pCerr;
	freopen_s(&pCout, "CONOUT$", "w", stdout);
	freopen_s(&pCerr, "CONOUT$", "w", stderr);

	if (__argc <= 1)
	{
		printf("No command line arguments provided!\n");
		printf("The first argument should be the path to an image and the second argument is the desired approximation error (default = 1.0)\n");
		return 0;
	}

	// the resource path is going to be our start up folder
	g_ResourcePath = __argv[0];
	while (!g_ResourcePath.empty() && g_ResourcePath[g_ResourcePath.size() - 1] != '\\')
		g_ResourcePath.pop_back();

	printf("Press 'o' to show/hide the the user interface.\n");
	printf("Press 'p' to export a screenshot to the working directory.\n");
	printf("Press 'Esc' to close the window.\n\n");

	{
		// read desired approximation error if available, otherwise use a default value
		float refineVarianceThreshold = 1.0;
		if (__argc > 2)
			refineVarianceThreshold = (float)atof(__argv[2]);

		// read the input image
		Image2D image(__argv[1]);
		Vec2i res = image.GetResolution();
		BoundingBox2d boundingbox(Vec2f(0, 0), res.asfloat());
		int width = res.x(), height = res.y();

		// create a viewer
		_ImageViewer = new Image2DView(res, boundingbox);
		_ImageViewer->SetImage(&image, NULL);

		// create the camera
		_Camera2D = new Camera2D(boundingbox);

		// create the triangulator
		_Triangulator = new Triangulator(_ImageViewer, refineVarianceThreshold);


		
		// ---------------------------------------
		// Create the window
		// ---------------------------------------
		HWND hWnd;
		if (FAILED(InitWindow(hInstance, nCmdShow, "Image Triangulation", width, height, hWnd)))
			return -1;

		// ---------------------------------------
		// Initialize D3D
		// ---------------------------------------
		_D3D = new D3D(hWnd);
		_D3D->SetCreateDevice_Callback(D3DCreateDevice);
		_D3D->SetCreateSwapChain_Callback(D3DCreateSwapChain);
		_D3D->SetReleaseDevice_Callback(D3DReleaseDevice);
		_D3D->SetReleaseSwapChain_Callback(D3DReleaseSwapChain);
		
		// ---------------------------------------
		// create resources
		// ---------------------------------------
		if (!_D3D->Init()) return -1;

		// ---------------------------------------
		// Initialize UI
		// ---------------------------------------
		_UI = new UI(_D3D, hWnd);
		if (!_UI->Init()) return -1;
		_UIBar = _UI->CreateBar("Options", 200, 300);
	
		_UIBar->AddButton("ExportBackbuffer", "ExportBackbuffer", ExportBackbuffer, _D3D, "Exports the backbuffer.", "");

		if (_Triangulator)
			_Triangulator->InitUI(_UI, _UIBar);

		_UIBar->Minimize();

		// ---------------------------------------
		// Enter the main loop
		// ---------------------------------------
		MSG msg = {0};
		while( WM_QUIT != msg.message )
		{
			if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
	
			if (IsKeyDown(VK_ESCAPE) != 0) break;	// close window on 'ESC'
			if (IsKeyPressed('O') != 0) _UI->IsVisible() ? _UI->Hide() : _UI->Show();	// toggle UI visibility on 'o'
			if (IsKeyPressed('P') != 0) ExportBackbuffer(_D3D);

			Render();
		}

		// ---------------------------------------
		// Clean up
		// ---------------------------------------
		SAFE_DELETE(_UIBar);
		SAFE_DELETE(_UI);
		SAFE_DELETE(_ImmediateRenderer2D);
		SAFE_DELETE(_ImageViewer);
		SAFE_DELETE(_Triangulator);
		SAFE_DELETE(_D3D);
		SAFE_DELETE(_Camera2D);
	}
	return 0;
}