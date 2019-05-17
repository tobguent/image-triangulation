#define NUM_THREADS 512

ByteAddressBuffer PlaneAccumCoeffs : register(t0);
RWByteAddressBuffer UavEquations : register(u0);

float3x3 chol(float3x3 A)
{
	float M[9] = {
		A._m00, A._m01, A._m02,
		A._m10, A._m11, A._m12,
		A._m20, A._m21, A._m22 };
	// compute cholesky decomposition
	float L[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < (i + 1); j++) {
			float s = 0;
			for (int k = 0; k < j; k++)
				s += L[i * 3 + k] * L[j * 3 + k];
			L[i * 3 + j] = (i == j) ?
				sqrt(M[i * 3 + i] - s) :
				(1.0 / L[j * 3 + j] * (M[i * 3 + j] - s));
		}
	}
	return float3x3(L[0], L[1], L[2],
		L[3], L[4], L[5],
		L[6], L[7], L[8]);
}

float3 cholsolve(float3x3 mL, float3 b)
{
	float3 y = float3(b.x / mL._m00,
		b.y / mL._m11 - (b.x*mL._m10) / (mL._m00*mL._m11),
		(b.x*mL._m10*mL._m21) / (mL._m00*mL._m11*mL._m22) - (b.y*mL._m21) / (mL._m11*mL._m22) - (b.x*mL._m20) / (mL._m00*mL._m22) + b.z / mL._m22);
	float3 x = float3(
		(mL._m10*mL._m21*y.z) / (mL._m00*mL._m11*mL._m22) - (mL._m20*y.z) / (mL._m00*mL._m22) - (mL._m10*y.y) / (mL._m00*mL._m11) + y.x / mL._m00,
		y.y / mL._m11 - (mL._m21*y.z) / (mL._m11*mL._m22),
		y.z / mL._m22);
	return x;
}

[numthreads(NUM_THREADS, 1, 1)]
void main( uint primitiveID : SV_DispatchThreadID )
{
	// grab coefficients of the linear systems
	uint4 coeffs1 = PlaneAccumCoeffs.Load4(primitiveID * 60 + 0);
	uint4 coeffs2 = PlaneAccumCoeffs.Load4(primitiveID * 60 + 16);
	uint4 coeffs3 = PlaneAccumCoeffs.Load4(primitiveID * 60 + 32);
	uint3 coeffs4 = PlaneAccumCoeffs.Load3(primitiveID * 60 + 48);

	// xx xy x  =  1 2 3  |  xR = 7  |  xG = 10  |  xB = 13
	//    yy y  =    4 5  |  yR = 8  |  yG = 11  |  yB = 14
	//       N  =      6  |  zR = 9  |  zG = 12  |  zB = 15

	uint xx = coeffs1.x;	uint xy = coeffs1.y;	uint x = coeffs1.z;
							uint yy = coeffs1.w;	uint y = coeffs2.x;
													uint N = coeffs2.y;
	uint xR = coeffs2.z;	uint yR = coeffs2.w;	uint zR = coeffs3.x;
	uint xG = coeffs3.y;	uint yG = coeffs3.z;	uint zG = coeffs3.w;
	uint xB = coeffs4.x;	uint yB = coeffs4.y;	uint zB = coeffs4.z;
	
	// system matrix
	float3x3 M = float3x3(
		xx, xy, x,
		xy, yy, y,
		x, y, N);
	float3x3 L = chol(M);

	// check if the systems is ill-posed or under-determined
	float3 abdR, abdG, abdB;
	const float EPS = 1E-5;
	if (L._m00 > EPS && L._m11 > EPS && L._m22 > EPS)
	{
		// right hand side
		float3 bR = float3(xR, yR, zR);
		float3 bG = float3(xG, yG, zG);
		float3 bB = float3(xB, yB, zB);

		// solve the three linear systems
		abdR = cholsolve(L, bR) / 255.0;
		abdG = cholsolve(L, bG) / 255.0;
		abdB = cholsolve(L, bB) / 255.0;
	}
	else
	{
		// fall back to a mean fit
		abdR = float3(0, 0, zR / (float)N / 255.0);
		abdG = float3(0, 0, zG / (float)N / 255.0);
		abdB = float3(0, 0, zB / (float)N / 255.0);
	}
	// store the resulting plane equations for R, G and B
	UavEquations.Store3(primitiveID * 36 + 0,  asuint(-abdR));
	UavEquations.Store3(primitiveID * 36 + 12, asuint(-abdG));
	UavEquations.Store3(primitiveID * 36 + 24, asuint(-abdB));
}
