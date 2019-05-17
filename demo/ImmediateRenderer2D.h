#pragma once

#include "math.hpp"
#include <vector>
#include <d3d11.h>
#include "effect-framework\d3dx11effect.h"

class Camera2D;

class ImmediateRenderer2D
{
	public:

		ImmediateRenderer2D(int bufferSize = 10 * 1024);
		~ImmediateRenderer2D();

		bool D3DCreateDevice(ID3D11Device* Device);
		void D3DReleaseDevice();

		void DrawLineStrip(const std::vector<Vec2f>& pnts, ID3D11DeviceContext* DeviceContext, Camera2D* camera);

		Vec4f Color;

	private:

		void Internal_Bind(ID3D11DeviceContext* DeviceContext, Camera2D* camera, const std::string& techniqueName);
		void Internal_SubmitCall(ID3D11DeviceContext* DeviceContext, const std::vector<Vec2f>& vertexBuffer, D3D11_PRIMITIVE_TOPOLOGY topology);
		void Internal_Unbind(ID3D11DeviceContext* DeviceContext);

		ID3D11Buffer* _VbPosition;
		ID3D11InputLayout* _InputLayoutPos;
		ID3DX11Effect* _Fx;

		int _BufferSize;
		int _Offset;
};