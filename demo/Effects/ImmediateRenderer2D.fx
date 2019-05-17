#include "ShaderCommon.hlsl"

row_major float4x4 View;
row_major float4x4 Projection; 
float4 Color = float4(1,0,0,1);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Point Rendering
/////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 VSSimple(float2 Position : POSITION) : SV_POSITION
{
	return mul(mul( float4(Position,0.0,1.0), View), Projection );
}

float4 PSSimple() : SV_Target0
{   
	return Color;
}

//--------------------------------------------------------------------------------------
technique11 Technique_Point
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VSSimple() ) );
        SetGeometryShader( 0 );
        SetPixelShader( CompileShader( ps_5_0, PSSimple() ) );
    }
}