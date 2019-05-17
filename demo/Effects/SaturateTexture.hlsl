#define NUM_THREADS 32

RWTexture2D<float> output : register(u0);

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void main( uint2 pixel : SV_DispatchThreadID )
{
	output[pixel] = saturate(output[pixel]);
}
