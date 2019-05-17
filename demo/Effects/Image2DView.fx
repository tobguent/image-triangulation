
row_major matrix View;
row_major matrix Projection;
Texture2D Image;
float2 DomainMin = {-1,-1};
float2 DomainMax = {1,1};

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = WRAP;
	AddressV = WRAP;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
float4 VSToScreen(uint vertexId : SV_VertexID) : SV_POSITION
{
	return float4((int)(vertexId / 2) * 2 - 1, (int)(1 - vertexId % 2) * 2 - 1, 0, 1);
}

float4 VSToDomain(uint vertexId : SV_VertexID, out float2 tx : TEXCOORD) : SV_POSITION
{
	tx = float2((int)(vertexId / 2), (int)(1 - vertexId % 2));
	return mul( mul( float4(lerp(DomainMin, DomainMax, tx), 0, 1), View ), Projection);
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSToScreen(float4 Position : SV_POSITION) : SV_Target
{
	return float4(Image.Load(int3(Position.xy, 0)).rgb, 1);
}

float4 PSToScreenSingleComponent(float4 Position : SV_POSITION) : SV_Target
{
	return float4(Image.Load(int3(Position.xy, 0)).rrr, 1);
}

float4 PSToDomain(float4 Position : SV_POSITION, float2 tx : TEXCOORD) : SV_Target
{
	return float4(Image.Sample(linearSampler, tx).rgb, 1);
}

//--------------------------------------------------------------------------------------
technique10 ToScreen
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_4_1, VSToScreen() ) );
		SetGeometryShader( 0 );
		SetPixelShader( CompileShader( ps_4_1, PSToScreen() ) );
	}
}

//--------------------------------------------------------------------------------------
technique10 ToScreenSingleComponent
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_1, VSToScreen()));
		SetGeometryShader(0);
		SetPixelShader(CompileShader(ps_4_1, PSToScreenSingleComponent()));
	}
}

//--------------------------------------------------------------------------------------
technique10 ToDomain
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_4_1, VSToDomain() ) );
		SetGeometryShader( 0 );
		SetPixelShader( CompileShader( ps_4_1, PSToDomain() ) );
	}
}
