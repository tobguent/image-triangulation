
#ifndef MSAA_SAMPLES
// uncommend this line to globally disable multi sampling
#define MSAA_SAMPLES 4
#endif

#ifndef _WIN32
#ifdef MSAA_SAMPLES

#if MSAA_SAMPLES == 1

static const float2 v2MSAAOffsets[1] =
{
    float2(0.0, 0.0)
};

#endif

#if MSAA_SAMPLES == 2

static const float2 v2MSAAOffsets[2] = 
{ 
    float2(0.25, 0.25),    float2(-0.25, -0.25) 
};

#endif

#if MSAA_SAMPLES == 4

static const float2 v2MSAAOffsets[4] = 
{ 
    float2(-0.125, -0.375),    float2(0.375, -0.125),
    float2(-0.375,  0.125),    float2(0.125,  0.375)
};

#endif

#if MSAA_SAMPLES == 8

static const float2 v2MSAAOffsets[8] = 
{ 
    float2(0.0625, -0.1875),    float2(-0.0625,  0.1875),
    float2(0.3125,  0.0625),    float2(-0.1875, -0.3125),
    float2(-0.3125,  0.3125),   float2(-0.4375, -0.0625),
    float2(0.1875,  0.4375),    float2(0.4375, -0.4375)
};

#endif

float2 getMSAATexCoord(float2 inTex, uint sampleIndex)
{
	// We need the texture address gradients in order to calulate the MSAA texturing offsets
    float2 v2DDX = ddx( inTex );
    float2 v2DDY = ddy( inTex );
	// Calculate texture offset
    float2 v2TexelOffset = v2MSAAOffsets[sampleIndex].x * v2DDX + v2MSAAOffsets[sampleIndex].y * v2DDY;
	return inTex + v2TexelOffset;
}

float3 getMSAATexCoord(float3 inTex, uint sampleIndex)
{
	// We need the texture address gradients in order to calulate the MSAA texturing offsets
	float3 v2DDX = ddx(inTex);
	float3 v2DDY = ddy(inTex);
	// Calculate texture offset
	float3 v2TexelOffset = v2MSAAOffsets[sampleIndex].x * v2DDX + v2MSAAOffsets[sampleIndex].y * v2DDY;
	return inTex + v2TexelOffset;
}

#endif
#endif