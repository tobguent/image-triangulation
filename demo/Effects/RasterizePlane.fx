#include "ShaderCommon.hlsl"

row_major matrix View;
row_major matrix Projection;

float4 Diffuse = {0.9,0.4,0.2,1};

globallycoherent RWByteAddressBuffer PlaneAccumCoeffs : register(u1);
ByteAddressBuffer SrvEquations;

Texture2D Image;
int2 ScreenResolution;
int2 TexResolution;
int NumTriangles;

SamplerState linearSampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = WRAP;
	AddressV = WRAP;
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float2 Pos : POSITION0;
};

struct GS_INPUT
{
	float4 Pos : POSITION0;
	unsigned int VertexID : VERTEXID;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
	int TriangleID : TRIANGLEID;
	int LayerID : LAYERID;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
GS_INPUT VS_Color(VS_INPUT input, unsigned int vertexID : SV_VertexID)
{
	GS_INPUT output = (GS_INPUT)0;
	output.Pos = mul(mul(float4(input.Pos, 0, 1), View), Projection);
	output.VertexID = vertexID;
	return output;
}

float2 sanitize(float2 pos, float2 delta)
{
	float2 pxwidth = 2.0 / float2(ScreenResolution);
	// keep positions on the boundary
	if (pos.x < -1 + pxwidth.x || pos.x > 1 - pxwidth.x)
		delta.x = 0;
	if (pos.y < -1 + pxwidth.y || pos.y > 1 - pxwidth.y)
		delta.y = 0;

	// do a gradient descent step
	pos += delta;

	// clamp after step to boundary
	pos.x = min(max(-1, pos.x), 1);
	pos.y = min(max(-1, pos.y), 1);
	return pos;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------
[maxvertexcount(39)]
void GS_Color(triangle GS_INPUT input[3], unsigned int primitiveID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> output)
{
	PS_INPUT vertex;
	vertex.TriangleID = primitiveID;

	float2 dx = float2(2.0 / float(ScreenResolution.x), 0);
	float2 dy = float2(0, 2.0 / float(ScreenResolution.y));

	// //////////////////////
	vertex.LayerID = 0;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 1;
	// ======================
	vertex.Pos = float4(sanitize(input[0].Pos.xy, dx), input[0].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 2;
	// ======================
	vertex.Pos = float4(sanitize(input[0].Pos.xy, -dx), input[0].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 3;
	// ======================
	vertex.Pos = float4(sanitize(input[0].Pos.xy, dy), input[0].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 4;
	// ======================
	vertex.Pos = float4(sanitize(input[0].Pos.xy, -dy), input[0].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 5;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[1].Pos.xy, dx), input[1].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 6;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[1].Pos.xy, -dx), input[1].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 7;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[1].Pos.xy, dy), input[1].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 8;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[1].Pos.xy, -dy), input[1].Pos.zw);
	output.Append(vertex);
	// ------------
	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 9;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[2].Pos.xy, dx), input[2].Pos.zw);
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 10;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[2].Pos.xy, -dx), input[2].Pos.zw);
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 11;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[2].Pos.xy, dy), input[2].Pos.zw);
	output.Append(vertex);
	output.RestartStrip();
	// ======================
	vertex.LayerID = 12;
	// ======================
	vertex.Pos = input[0].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = input[1].Pos;
	output.Append(vertex);
	// ------------
	vertex.Pos = float4(sanitize(input[2].Pos.xy, -dy), input[2].Pos.zw);
	output.Append(vertex);
	output.RestartStrip();
	// ======================
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
void PS( PS_INPUT input)
{
	// get texture coordinate
	float2 tx = input.Pos.xy / float2(ScreenResolution);
	tx.y = 1 - tx.y;

	// sample texture from background
	float3 color = Image.Sample(linearSampler, tx).xyz;
	uint3 ucolor = uint3(color * 255);
		
	uint x = input.Pos.x;
	uint y = input.Pos.y;

	// xx xy x  =  1 2 3  |  xR = 7  |  xG = 10  |  xB = 13
	//    yy y  =    4 5  |  yR = 8  |  yG = 11  |  yB = 14
	//       N  =      6  |  zR = 9  |  zG = 12  |  zB = 15
	// add to respective triangle!
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 0,  x*x);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 4,  x*y);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 8,  x);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 12, y*y);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 16, y);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 20, 1);
													  
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 24, x*ucolor.x);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 28, y*ucolor.x);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 32, ucolor.x);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 36, x*ucolor.y);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 40, y*ucolor.y);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 44, ucolor.y);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 48, x*ucolor.z);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 52, y*ucolor.z);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 60 + 56, ucolor.z);
}

void PS_Var(PS_INPUT input)
{
	// get texture coordinate
	float2 tx = input.Pos.xy / float2(ScreenResolution);
	tx.y = 1 - tx.y;

	// sample texture from background
	float3 color = Image.Sample(linearSampler, tx).xyz;
	uint3 ucolor = uint3(color * 255);

	// look up the plane equations
	float3 eqsR = asfloat(SrvEquations.Load3((input.TriangleID * 13 + input.LayerID) * 36 + 0));
	float3 eqsG = asfloat(SrvEquations.Load3((input.TriangleID * 13 + input.LayerID) * 36 + 12));
	float3 eqsB = asfloat(SrvEquations.Load3((input.TriangleID * 13 + input.LayerID) * 36 + 24));

	float x = (int)input.Pos.x;
	float y = (int)input.Pos.y;
	float3 colorPlane = float3(
		-(eqsR.x * x + eqsR.y * y + eqsR.z),
		-(eqsG.x * x + eqsG.y * y + eqsG.z),
		-(eqsB.x * x + eqsB.y * y + eqsB.z)) * 255.0;

	// compute variance
	float scale = 10;
	uint3 vari = uint3(
		(ucolor.x - colorPlane.x)*(ucolor.x - colorPlane.x)*scale, 
		(ucolor.y - colorPlane.y)*(ucolor.y - colorPlane.y)*scale, 
		(ucolor.z - colorPlane.z)*(ucolor.z - colorPlane.z)*scale);

	// add to respective triangle!
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 0, vari.x);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 4, vari.y);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 8, vari.z);
	PlaneAccumCoeffs.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 12, 1);
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique11 RenderCoefficients
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_Color()));
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}

technique11 RenderMeanError
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_Color()));
		SetPixelShader(CompileShader(ps_5_0, PS_Var()));
	}
}
