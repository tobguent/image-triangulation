
row_major matrix View;
row_major matrix Projection;
Texture2D Image;
float2 DomainMin = {-1,-1};
float2 DomainMax = {1,1};

float InkAmount = 0.1;

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
float4 VSToDomain(uint vertexId : SV_VertexID, out float2 tx : TEXCOORD) : SV_POSITION
{
	tx = float2((int)(vertexId / 2), (int)(1 - vertexId % 2));
	return mul( mul( float4(lerp(DomainMin, DomainMax, tx), 0, 1), View ), Projection);
}

float4 VSToScreen(uint vertexId : SV_VertexID) : SV_POSITION
{
	return float4((int)(vertexId / 2) * 2 - 1, (int)(1 - vertexId % 2) * 2 - 1, 0, 1);
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 PSToDomainNormal(float4 Position : SV_POSITION, float2 tx : TEXCOORD) : SV_Target
{
	float2 c = float2(0.5, 0.5);
	float2 d = tx - c;
	if (dot(d, d) <= 0.25)
	{
		float r = length(d);
		float sigma = 0.5 / 2;
		float pi = 3.14159265359;
		float w = 1 / sqrt(2 * pi*sigma*sigma) * exp(-r*r / (2 * sigma*sigma));
		return float4(w, w, w, w) * InkAmount;
	}
	else
		discard;
	return float4(0, 0, 0, 0);
}

float4 PSToScreen(float4 Position : SV_POSITION) : SV_Target
{
	float c = Image.Load(int3(Position.xy, 0)).r;
	float cx = Image.Load(int3(Position.xy + int2(1,0), 0)).r;
	float cy = Image.Load(int3(Position.xy + int2(0,1), 0)).r;
	float t = 0.5;
	if ((c<t && cx>t) || (c > t && cx < t)
		|| (c<t && cy>t) || (c > t && cy < t))
		return float4(1, 1, 1, 1);
	else discard;
	return float4(0, 0, 0, 0);
}

float4 PSToScreenBackground(float4 Position : SV_POSITION) : SV_Target
{
	float c = Image.Load(int3(Position.xy, 0)).r;
	float t = 0.5;
	float sub = 0.3;
	if (c < t)
		return float4(sub, sub, sub, sub);
	return float4(0, 0, 0, 0);
}

//--------------------------------------------------------------------------------------
technique10 ToDomainNormal
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_1, VSToDomain()));
		SetGeometryShader(0);
		SetPixelShader(CompileShader(ps_4_1, PSToDomainNormal()));
	}
}

//--------------------------------------------------------------------------------------
technique10 ToScreenIsolines
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_1, VSToScreen()));
		SetGeometryShader(0);
		SetPixelShader(CompileShader(ps_4_1, PSToScreen()));
	}
}

technique10 ToScreenBackground
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_1, VSToScreen()));
		SetGeometryShader(0);
		SetPixelShader(CompileShader(ps_4_1, PSToScreenBackground()));
	}
}