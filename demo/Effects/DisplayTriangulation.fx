#include "ShaderCommon.hlsl"

row_major matrix View;
row_major matrix Projection;

float4 Diffuse = {0.9,0.4,0.2,1};

float2 VarianceBounds = { 0, 1 };
int ViewLayerID = 0;

ByteAddressBuffer ColorData;
ByteAddressBuffer PlaneEquations;

Texture2D Image;
int2 TexResolution;
int2 ScreenResolution;

float VarianceScale = 1;

SamplerState linearSampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = WRAP;
	AddressV = WRAP;
};

float3 TransferFunction(float t) {
	if (t > VarianceBounds.y)
		return float3(1, 0, 1);
	
	t = 1 - saturate((t - VarianceBounds.x) / (VarianceBounds.y - VarianceBounds.x));
	float3 color =
		(((float3(5.0048, 7.4158, 6.1246)*t +
			float3(-8.0915, -15.9415, -16.2287))*t +
			float3(1.1657, 7.4696, 11.9910))*t +
			float3(1.4380, 1.2767, -1.4886))*t +
		float3(0.6639, -0.0013, 0.1685);
	return color;
}

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float2 Pos : POSITION0;
};

struct GS_INPUT
{
	float4 Pos : POSITION0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
	float3 Color : COLOR0;
	float3 Bary : BARYCENTERIC;
};

struct PS_INPUT_COLORPLANE
{
	float4 Pos : SV_POSITION;
	float3 EqsR : EQSR;
	float3 EqsG : EQSG;
	float3 EqsB : EQSB;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS_Wire( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( mul( float4(input.Pos,0,1), View ), Projection);
	output.Color = Diffuse.xyz;
    return output;
}

GS_INPUT VS_Color(VS_INPUT input)
{
	GS_INPUT output = (GS_INPUT)0;
	output.Pos = mul(mul(float4(input.Pos, 0, 1), View), Projection);
	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------
[maxvertexcount(3)]
void GS_Color(triangle GS_INPUT input[3], unsigned int primitiveID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> output)
{
	float4 accumColor = float4(ColorData.Load4((primitiveID * 13 + ViewLayerID) * 16)) / float4(255, 255, 255, 1);
	PS_INPUT vertex;
	vertex.Color = accumColor.xyz / accumColor.w;

	vertex.Pos = input[0].Pos;
	vertex.Bary = float3(1, 0, 0);
	output.Append(vertex);

	vertex.Pos = input[1].Pos;
	vertex.Bary = float3(0, 1, 0);
	output.Append(vertex);

	vertex.Pos = input[2].Pos;
	vertex.Bary = float3(0, 0, 1);
	output.Append(vertex);
}

[maxvertexcount(3)]
void GS_Variance(triangle GS_INPUT input[3], unsigned int primitiveID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> output)
{
	float4 accumColor = float4(ColorData.Load4((primitiveID * 13 + ViewLayerID) * 16 )) / float4(255*255, 255*255, 255*255, 1);
	PS_INPUT vertex;
	float3 variance = accumColor.xyz / VarianceScale; // / accumColor.w;
	vertex.Color = TransferFunction(255 * sqrt( (variance.x + variance.y + variance.z) / 3.0 / (TexResolution.x*TexResolution.y)));

	vertex.Pos = input[0].Pos;
	vertex.Bary = float3(1, 0, 0);
	output.Append(vertex);

	vertex.Pos = input[1].Pos;
	vertex.Bary = float3(0, 1, 0);
	output.Append(vertex);

	vertex.Pos = input[2].Pos;
	vertex.Bary = float3(0, 0, 1);
	output.Append(vertex);
}

[maxvertexcount(3)]
void GS_ColorPlane(triangle GS_INPUT input[3], unsigned int primitiveID : SV_PrimitiveID, inout TriangleStream<PS_INPUT_COLORPLANE> output)
{
	PS_INPUT_COLORPLANE vertex;
	vertex.EqsR = asfloat(PlaneEquations.Load3((primitiveID * 13 + ViewLayerID) * 36 + 0));
	vertex.EqsG = asfloat(PlaneEquations.Load3((primitiveID * 13 + ViewLayerID) * 36 + 12));
	vertex.EqsB = asfloat(PlaneEquations.Load3((primitiveID * 13 + ViewLayerID) * 36 + 24));

	vertex.Pos = input[0].Pos;
	output.Append(vertex);

	vertex.Pos = input[1].Pos;
	output.Append(vertex);

	vertex.Pos = input[2].Pos;
	output.Append(vertex);
	output.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	return float4(input.Color, 1);
}

float4 PS_Tex(PS_INPUT input) : SV_Target
{
	// get texture coordinate
	float2 tx = input.Pos.xy / float2(ScreenResolution);
	tx.y = 1 - tx.y;

	// sample texture from background
	float3 color = Image.Sample(linearSampler, tx).xyz;
	//color = float3(0.9, 0.5, 0.2);

	return float4(color, 1);
}

float4 PS_ColorPlane(PS_INPUT_COLORPLANE input) : SV_Target
{
	// look up the plane equations
	float x = ((int)input.Pos.x) / (float)ScreenResolution.x * TexResolution.x;
float y = ((int)input.Pos.y) / (float)ScreenResolution.y * TexResolution.y;
float3 colorPlane = float3(
	-(input.EqsR.x * x + input.EqsR.y * y + input.EqsR.z),
	-(input.EqsG.x * x + input.EqsG.y * y + input.EqsG.z),
	-(input.EqsB.x * x + input.EqsB.y * y + input.EqsB.z));

return float4(colorPlane, 1);
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique11 RenderWire
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_5_0, VS_Wire() ) );
		SetGeometryShader( NULL );		
		SetPixelShader( CompileShader( ps_5_0, PS() ) );
	}
}

technique11 RenderColor
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_Color()));
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}

technique11 RenderColorPlane
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_ColorPlane()));
		SetPixelShader(CompileShader(ps_5_0, PS_ColorPlane()));
	}
}

technique11 RenderVariance
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_Variance()));
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}

technique11 RenderTex
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS_Color()));
		SetGeometryShader(CompileShader(gs_5_0, GS_Color()));
		SetPixelShader(CompileShader(ps_5_0, PS_Tex()));
	}
}
