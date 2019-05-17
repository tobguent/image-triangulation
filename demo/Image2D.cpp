#include "stdafx.h"
#include "Image2D.h"
#include "D3D.h"
#include <iostream>
#include <vector>
#include <fstream>
#include "MatlabInterface.hpp"

void Image2D::ExportBmp(const char* filename) const
{
	unsigned char file[14] = {
		'B', 'M', // magic
		0, 0, 0, 0, // size in bytes
		0, 0, // app data
		0, 0, // app data
		40 + 14, 0, 0, 0 // start of data offset
	};
	unsigned char info[40] = {
		40, 0, 0, 0, // info hd size
		0, 0, 0, 0, // width
		0, 0, 0, 0, // heigth
		1, 0, // number color planes
		24, 0, // bits per pixel
		0, 0, 0, 0, // compression is none
		0, 0, 0, 0, // image bits size
		0x13, 0x0B, 0, 0, // horz resoluition in pixel / m
		0x13, 0x0B, 0, 0, // vert resolutions (0x03C3 = 96 dpi, 0x0B13 = 72 dpi)
		0, 0, 0, 0, // #colors in pallete
		0, 0, 0, 0, // #important colors
	};

	int w = _ScreenResolution.x();
	int h = _ScreenResolution.y();

	int padSize = (4 - (w * 3) % 4) % 4;
	int sizeData = w*h * 3 + h*padSize;
	int sizeAll = sizeData + sizeof(file) + sizeof(info);

	file[2] = (unsigned char)(sizeAll);
	file[3] = (unsigned char)(sizeAll >> 8);
	file[4] = (unsigned char)(sizeAll >> 16);
	file[5] = (unsigned char)(sizeAll >> 24);

	info[4] = (unsigned char)(w);
	info[5] = (unsigned char)(w >> 8);
	info[6] = (unsigned char)(w >> 16);
	info[7] = (unsigned char)(w >> 24);

	info[8] = (unsigned char)(h);
	info[9] = (unsigned char)(h >> 8);
	info[10] = (unsigned char)(h >> 16);
	info[11] = (unsigned char)(h >> 24);

	info[20] = (unsigned char)(sizeData);
	info[21] = (unsigned char)(sizeData >> 8);
	info[22] = (unsigned char)(sizeData >> 16);
	info[23] = (unsigned char)(sizeData >> 24);

	std::ofstream stream(filename, std::ios::binary | std::ios::out);
	stream.write((char*)file, sizeof(file));
	stream.write((char*)info, sizeof(info));

	unsigned char pad[3] = { 0, 0, 0 };

	for (int y = 0; y<h; y++)
	{
		for (int x = 0; x<w; x++)
		{
			Vec3d color = _Color[y*w + x];
			unsigned char pixel[3];
			pixel[0] = (unsigned char)(std::min(std::max(0.f, (float)color.z()), 1.f) * 255);
			pixel[1] = (unsigned char)(std::min(std::max(0.f, (float)color.y()), 1.f) * 255);
			pixel[2] = (unsigned char)(std::min(std::max(0.f, (float)color.x()), 1.f) * 255);

			stream.write((char*)pixel, 3);
		}
		stream.write((char*)pad, padSize);
	}
}

static void skipChars(FILE* infile, int numChars)
{
	for (int i = 0; i<numChars; i++) {
		fgetc(infile);
	}
}

static short readShort(FILE* infile)
{
	// read a 16 bit integer
	unsigned char lowByte, hiByte;
	lowByte = fgetc(infile);         // Read the low order byte (little endian form)
	hiByte = fgetc(infile);         // Read the high order byte

									// Pack together
	short ret = hiByte;
	ret <<= 8;
	ret |= lowByte;
	return ret;
}

static long readLong(FILE* infile)
{
	// Read in 32 bit integer
	unsigned char byte0, byte1, byte2, byte3;
	byte0 = fgetc(infile);         // Read bytes, low order to high order
	byte1 = fgetc(infile);
	byte2 = fgetc(infile);
	byte3 = fgetc(infile);

	// Pack together
	long ret = byte3;
	ret <<= 8;
	ret |= byte2;
	ret <<= 8;
	ret |= byte1;
	ret <<= 8;
	ret |= byte0;
	return ret;
}

static long GetNumBytesPerRow(int NumCols) { return ((3 * NumCols + 3) >> 2) << 2; }

bool Image2D::ImportBmp(const char* filename)
{
	auto s1 = util::MatlabInterface::meval("Image=imread('" + std::string(filename) + "'); R=double(Image(:,:,1)); G=double(Image(:,:,2)); B=double(Image(:,:,3));");
	auto s2 = util::MatlabInterface::meval("resX = double(size(Image,2));");
	auto s3 = util::MatlabInterface::meval("resY = double(size(Image,1));");
	Vec2d res;
	util::MatlabInterface::getScalar<double>("resX", res.x());
	util::MatlabInterface::getScalar<double>("resY", res.y());
	_ScreenResolution = Vec2i((int)res.x(), (int)res.y());

	// read the matrices back
	Eigen::MatrixXd dataR = util::MatlabInterface::getEigenMatrix<double, double>("R");
	Eigen::MatrixXd dataG = util::MatlabInterface::getEigenMatrix<double, double>("G");
	Eigen::MatrixXd dataB = util::MatlabInterface::getEigenMatrix<double, double>("B");

	_Color.resize(_ScreenResolution.x() * _ScreenResolution.y());
	for (int y = 0; y < _ScreenResolution.y(); ++y)
		for (int x = 0; x < _ScreenResolution.x(); ++x)
			_Color[(_ScreenResolution.y()-1-y)*_ScreenResolution.x()+x] = Vec3d(dataR(y,x), dataG(y,x), dataB(y,x)) / 255.f;

	return true;
}

Image2DView::Image2DView(const Vec2i& resolution, const BoundingBox2d& box) :
	_Resolution(resolution),
	_Effect(NULL),
	_Tex(NULL),
	_Srv(NULL),
	_MarkedDirty(false),
	_BoundingBox(box)
{
	_Data.resize(resolution.x() * resolution.y());
}

Image2DView::~Image2DView()
{
	D3DReleaseDevice();
}

bool Image2DView::D3DCreateDevice(ID3D11Device* Device)
{
	if (!D3D::LoadEffectFromFile("Image2DView.fxo", Device, &_Effect)) return false;

	{
		// create the 2D texture of the earth
		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D11_TEXTURE2D_DESC));
		texDesc.Width = _Resolution.x();
		texDesc.Height = _Resolution.y();
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.SampleDesc.Count = 1;
		texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		texDesc.Usage = D3D11_USAGE_DYNAMIC;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA init;
		ZeroMemory(&init, sizeof(D3D11_SUBRESOURCE_DATA));
		init.pSysMem = &_Data[0];
		init.SysMemPitch = sizeof(Vec4f) * _Resolution.x();

		if (FAILED(Device->CreateTexture2D(&texDesc, &init, &_Tex)))
		{
			std::cerr << "Image2DView: failed to create earth texture" << std::endl;
			return false;
		}
		if (FAILED(Device->CreateShaderResourceView(_Tex, NULL, &_Srv)))
		{
			std::cerr << "Image2DView: failed to create shader resource view" << std::endl;
			return false;
		}
	}

	return true;
}

void Image2DView::D3DReleaseDevice()
{
	SAFE_RELEASE(_Effect);
	SAFE_RELEASE(_Tex);
	SAFE_RELEASE(_Srv);
}

void Image2DView::DrawToScreen(ID3D11DeviceContext* ImmediateContext)
{
	UpdateGPUResources(ImmediateContext);
	DrawToScreen(ImmediateContext, _Srv, false);
}

void Image2DView::DrawToScreen(ID3D11DeviceContext* ImmediateContext, ID3D11ShaderResourceView* otherSrv, bool singleComponent)
{
	float factor[] = { 1,1,1,1 };
	ID3D11Buffer* noVBs[] = { NULL };
	unsigned int strides[] = { 0 };
	unsigned int offsets[] = { 0 };
	ImmediateContext->IASetVertexBuffers(0, 1, noVBs, strides, offsets);
	ImmediateContext->IASetInputLayout(NULL);
	ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ImmediateContext->RSSetState(RasterizerStates::CullNone());
	ImmediateContext->OMSetBlendState(BlendStates::Default(), factor, 0xFF);

	_Effect->GetVariableByName("Image")->AsShaderResource()->SetResource(otherSrv);
	if (singleComponent)
		_Effect->GetTechniqueByName("ToScreenSingleComponent")->GetPassByIndex(0)->Apply(0, ImmediateContext);
	else _Effect->GetTechniqueByName("ToScreen")->GetPassByIndex(0)->Apply(0, ImmediateContext);
	ImmediateContext->Draw(4, 0);

	ID3D11ShaderResourceView* noSrv[1] = { NULL };
	ImmediateContext->PSSetShaderResources(0, 1, noSrv);
}

void Image2DView::DrawToDomain(ID3D11DeviceContext* ImmediateContext, Camera2D* camera)
{
	UpdateGPUResources(ImmediateContext);

	float factor[] = { 1,1,1,1 };
	ID3D11Buffer* noVBs[] = { NULL };
	unsigned int strides[] = { 0 };
	unsigned int offsets[] = { 0 };
	ImmediateContext->IASetVertexBuffers(0, 1, noVBs, strides, offsets);
	ImmediateContext->IASetInputLayout(NULL);
	ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ImmediateContext->RSSetState(RasterizerStates::CullNone());
	ImmediateContext->OMSetBlendState(BlendStates::Default(), factor, 0xFF);

	_Effect->GetVariableByName("View")->AsMatrix()->SetMatrix(camera->GetView().ptr());
	_Effect->GetVariableByName("Projection")->AsMatrix()->SetMatrix(camera->GetProj().ptr());
	_Effect->GetVariableByName("Image")->AsShaderResource()->SetResource(_Srv);
	float domainMin[] = { (float)_BoundingBox.GetMin().x(), (float)_BoundingBox.GetMin().y() };
	float domainMax[] = { (float)_BoundingBox.GetMax().x(), (float)_BoundingBox.GetMax().y() };
	_Effect->GetVariableByName("DomainMin")->AsVector()->SetFloatVector(domainMin);
	_Effect->GetVariableByName("DomainMax")->AsVector()->SetFloatVector(domainMax);
	_Effect->GetTechniqueByName("ToDomain")->GetPassByIndex(0)->Apply(0, ImmediateContext);

	ImmediateContext->Draw(4, 0);
}

void Image2DView::SetImage(Image2D* image, ID3D11DeviceContext* ImmediateContext)
{
	if (_Resolution != image->GetResolution()) {
		printf("ERROR! Viewport sizes do not match! Can't set the image data!\n");
		return;
	}
	//_BoundingBox = image->GetBoundingBox();

#ifdef NDEBUG
#pragma omp parallel
#endif
	{
#ifdef NDEBUG
#pragma omp for schedule(dynamic,1) //collapse(2)
#endif 
		for (int j = 0; j<_Resolution.y(); ++j)
		{
			for (int i = 0; i<_Resolution.x(); ++i)
			{
				_Data[j * _Resolution.x() + i] = Vec4f(image->GetPixel(Vec2i(i, j)).asfloat(), 1.0f);
			}
		}
	}//omp


	_MarkedDirty = true;

	//copy vertex indices
	if (ImmediateContext)
		UpdateGPUResources(ImmediateContext);
	//	else
	//		_MarkedDirty = true;
}

void Image2DView::UpdateGPUResources(ID3D11DeviceContext* ImmediateContext)
{
	if (_MarkedDirty && _Tex)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));
		if (SUCCEEDED(ImmediateContext->Map(_Tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		{
			BYTE* mappedData = reinterpret_cast<BYTE*>(mappedResource.pData); // destination
			BYTE* buffer = (BYTE*)&(_Data[0]);		// source
			int rowspan = _Resolution.x() * sizeof(Vec4f);	// row-pitch in source
			for (int y = 0; y < _Resolution.y(); ++y)		// copy in the data line by line
			{
				memcpy(mappedData, buffer, rowspan);
				mappedData += mappedResource.RowPitch;	// row-pitch in destination
				buffer += rowspan;
			}
			ImmediateContext->Unmap(_Tex, 0);
		}
		_MarkedDirty = false;
	}
}
