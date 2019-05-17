#include "ShaderCommon.hlsl"

row_major matrix View;
row_major matrix Projection;

float4 Diffuse = {0.9,0.4,0.2,1};

RWByteAddressBuffer AccumColor : register(u1);
ByteAddressBuffer SrvAccumColor;

Texture2D Image;
int2 ScreenResolution;

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
		
	// add to respective triangle!
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 0,  ucolor.x);
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 4,  ucolor.y);
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 8,  ucolor.z);
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 12, 1);
}

void PS_Var(PS_INPUT input)
{
	// get texture coordinate
	float2 tx = input.Pos.xy / float2(ScreenResolution);
	tx.y = 1 - tx.y;

	// sample texture from background
	float3 color = Image.Sample(linearSampler, tx).xyz;
	float3 ucolor = float3(color * 255);

	// look up the mean
	uint4 umean = SrvAccumColor.Load4((input.TriangleID * 13 + input.LayerID) * 16);
	float3 mean = umean.w == 0 ? uint3(0,0,0) : umean.xyz / (float)umean.w;

	// compute variance
	uint3 vari = uint3((ucolor.x - mean.x)*(ucolor.x - mean.x), (ucolor.y - mean.y)*(ucolor.y - mean.y), (ucolor.z - mean.z)*(ucolor.z - mean.z));

	// add to respective triangle!
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 0, vari.x);
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 4, vari.y);
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 8, vari.z);
	AccumColor.InterlockedAdd((input.TriangleID * 13 + input.LayerID) * 16 + 12, 1);
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique11 RenderMean
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_Color()));
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}

technique11 RenderVariance
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_Color()));
		SetPixelShader(CompileShader(ps_5_0, PS_Var()));
	}
}
