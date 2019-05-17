#define NUM_THREADS 512

ByteAddressBuffer input : register(t0);
RWByteAddressBuffer output : register(u0);

[numthreads(NUM_THREADS, 1, 1)]
void main( uint primitiveID : SV_DispatchThreadID )
{
	uint4 value = input.Load4((primitiveID * 13 + 0) * 16);
	output.Store4(primitiveID * 16, value);
}
