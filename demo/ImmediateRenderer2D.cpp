#include "stdafx.h"
#include "ImmediateRenderer2D.h"
#include "D3D.h"

ImmediateRenderer2D::ImmediateRenderer2D(int bufferSize) : 
_BufferSize(bufferSize),
_Offset(0),
Color(1,163.0f/255.0f,0,1),
_VbPosition(NULL),
_InputLayoutPos(NULL),
_Fx(NULL)
{
}

ImmediateRenderer2D::~ImmediateRenderer2D()
{
	D3DReleaseDevice();
}

bool ImmediateRenderer2D::D3DCreateDevice(ID3D11Device* Device)
{
	// --------------------------------------------
	// Create vertex buffer
	// --------------------------------------------
	{
		D3D11_BUFFER_DESC buffer;
		ZeroMemory(&buffer, sizeof(D3D11_BUFFER_DESC));
		buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer.Usage = D3D11_USAGE_DYNAMIC;
		buffer.ByteWidth = sizeof(Vec2f) * _BufferSize;
		if (FAILED(Device->CreateBuffer(&buffer, NULL, &_VbPosition))) return false;
	}

	// --------------------------------------------
	// Load shader
	// --------------------------------------------
	if (!D3D::LoadEffectFromFile("ImmediateRenderer2D.fxo", Device, &_Fx)) {
		printf("ImmediateRenderer2D: Couldn't load shader ImmediateRenderer2D.fx");
		return false;
	}
	
	// --------------------------------------------
	// Create input layout
	// --------------------------------------------
	{
		D3DX11_PASS_SHADER_DESC VsPassDesc;
		D3DX11_EFFECT_SHADER_DESC VsDesc;
		_Fx->GetTechniqueByName("Technique_Point")->GetPassByIndex(0)->GetVertexShaderDesc(&VsPassDesc);
		VsPassDesc.pShaderVariable->GetShaderDesc(VsPassDesc.ShaderIndex, &VsDesc);
		const D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if (FAILED(Device->CreateInputLayout( layout, 1, VsDesc.pBytecode, VsDesc.BytecodeLength, &_InputLayoutPos))) return false;
	}

	return true;
}

void ImmediateRenderer2D::D3DReleaseDevice()
{
	SAFE_RELEASE(_VbPosition);
	SAFE_RELEASE(_InputLayoutPos);
	SAFE_RELEASE(_Fx);
}

void ImmediateRenderer2D::DrawLineStrip(const std::vector<Vec2f>& pnts, ID3D11DeviceContext* DeviceContext, Camera2D* camera)
{
	if (pnts.empty()) return;
	Internal_Bind(DeviceContext, camera, "Technique_Point");
	Internal_SubmitCall(DeviceContext, pnts, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	Internal_Unbind(DeviceContext);
}

void ImmediateRenderer2D::Internal_Bind(ID3D11DeviceContext* DeviceContext, Camera2D* camera, const std::string& techniqueName)
{
	DeviceContext->IASetInputLayout(_InputLayoutPos);
	
	_Fx->GetVariableByName("View")->AsMatrix()->SetMatrix( camera->GetView().ptr() );
	_Fx->GetVariableByName("Projection")->AsMatrix()->SetMatrix( camera->GetProj().ptr() );
	
	_Fx->GetVariableByName("Color")->AsVector()->SetFloatVector( Color.ptr() );
	_Fx->GetTechniqueByName(techniqueName.c_str())->GetPassByIndex(0)->Apply(0, DeviceContext);
}

void ImmediateRenderer2D::Internal_SubmitCall(ID3D11DeviceContext* DeviceContext, const std::vector<Vec2f>& vertexBuffer, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	ID3D11Buffer* vbs[] = { _VbPosition };
	UINT strides[] = { 8 };
	UINT offsets[] = { 0 };
	DeviceContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
	DeviceContext->IASetPrimitiveTopology(topology);

	int queued = (int)vertexBuffer.size();
	int safeTotalSize = _BufferSize - 32;
	
	if (queued >= _BufferSize) {
		printf("ImmediateRenderer2D::Buffer to render too large! Splitting not yet implemented! Skipping this geometry!\n");
		return;
	}

	// if it still fits into the buffer append with no overwrite
	if (_Offset + queued < safeTotalSize)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(DeviceContext->Map(_VbPosition, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped)))
		{
			Vec2f* dst = (Vec2f*)mapped.pData;
			Vec2f* src = (Vec2f*)&vertexBuffer[0];
			int rawQueued = queued*sizeof(Vec2f);
			memcpy_s(dst + _Offset, rawQueued, src, rawQueued);

			DeviceContext->Unmap(_VbPosition, 0);
			DeviceContext->Draw(queued, _Offset);
		}
		_Offset += queued;
	}
	else // if it doesn't fit, restart (discard)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(DeviceContext->Map(_VbPosition, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			Vec2f* dst = (Vec2f*)mapped.pData;
			Vec2f* src = (Vec2f*)&vertexBuffer[0];
			int rawQueued = queued*sizeof(Vec2f);
			memcpy_s(dst, rawQueued, src, rawQueued);

			DeviceContext->Unmap(_VbPosition, 0);
			DeviceContext->Draw(queued, 0);
		}
		_Offset = queued;
	}
}

void ImmediateRenderer2D::Internal_Unbind(ID3D11DeviceContext* DeviceContext)
{
	ID3D11Buffer* vbs[] = { NULL, NULL };
	UINT strides[] = { 8, 12 };
	UINT offsets[] = { 0, 0 };
	DeviceContext->IASetVertexBuffers(0, 2, vbs, strides, offsets);
	DeviceContext->IASetInputLayout(NULL);
}
