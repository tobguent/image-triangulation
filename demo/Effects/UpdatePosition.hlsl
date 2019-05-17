#define NUM_THREADS 512

ByteAddressBuffer AdjTriangleEntry : register(t0);
ByteAddressBuffer AdjTriangleCount : register(t1);
ByteAddressBuffer AdjTriangleLists : register(t2);
ByteAddressBuffer TriVarianceColor : register(t3);
ByteAddressBuffer IndexBuffer	   : register(t4);

RWByteAddressBuffer VbPosition : register(u0);

cbuffer cbparam : register(b0)
{
	int NumVertices;
	float StepSize;
	float2 DomainSize;
	float UmbrellaDamping;
	float TrustRegion;
}

float3 to3(uint4 value, inout float weightsum)
{
	if (value.w > 0)
	{
		float weight = value.w;
		weightsum += weight;
		return float3(value.x, value.y, value.z) / float(value.w) * weight;
	}
	else return float3(0, 0, 0);
}

// intersects two line segments
bool intersect(float2 a0, float2 a1, float2 b0, float2 b1, out float2 st)
{
	float2x2 A = float2x2(a1.x - a0.x, b0.x - b1.x, a1.y - a0.y, b0.y - b1.y);
	float2 b = b0 - a0;
	float det = A._11*A._22 - A._12*A._21;
	if (det == 0) return false;

	float2x2 Ainv = float2x2(A._22, -A._12, -A._21, A._11) / det;
	st = mul(Ainv, b);
	return true;
}

[numthreads(NUM_THREADS, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	// get the vertex index (skip the first four vertices, since we want to keep the corners fixed)
	int vertexID = DTid;
	if (vertexID >= NumVertices) return;

	// get entry and count for the neighbor list
	uint entry = AdjTriangleEntry.Load(vertexID * 4);
	uint count = AdjTriangleCount.Load(vertexID * 4);

	// these variables will receive the sum of neighboring variances for the differnt offsets
	float3 vxp = float3(0, 0, 0);
	float3 vxm = float3(0, 0, 0);
	float3 vyp = float3(0, 0, 0);
	float3 vym = float3(0, 0, 0);
	float3 vc = float3(0, 0, 0);

	float wxp = 0, wyp = 0, wxm = 0, wym = 0;
	float minarea = 1000000000;
	float maxarea = 0;

	// iterate the neighbors
	for (uint poolIndex = entry; poolIndex < entry + count; ++poolIndex)
	{
		// extract triangle index and which corner we are
		int combinedIndex = AdjTriangleLists.Load(poolIndex * 4);
		int triIndex = combinedIndex / 3u;
		int cornerIndex = combinedIndex % 3u;

		float area=0;
		vc += to3(TriVarianceColor.Load4(((triIndex * 13) + 0) * 16), area);
		minarea = min(minarea, area);
		maxarea = max(maxarea, area);

		// read and sum up the variance
		vxp += to3(TriVarianceColor.Load4(((triIndex * 13) + 4 * cornerIndex + 1 + 0) * 16), wxp);
		vxm += to3(TriVarianceColor.Load4(((triIndex * 13) + 4 * cornerIndex + 1 + 1) * 16), wxm);
		vyp += to3(TriVarianceColor.Load4(((triIndex * 13) + 4 * cornerIndex + 1 + 2) * 16), wyp);
		vym += to3(TriVarianceColor.Load4(((triIndex * 13) + 4 * cornerIndex + 1 + 3) * 16), wym);
	}

	float fvxp = (vxp.x + vxp.y + vxp.z) / (3.0);
	float fvxm = (vxm.x + vxm.y + vxm.z) / (3.0);
	float fvyp = (vyp.x + vyp.y + vyp.z) / (3.0);
	float fvym = (vym.x + vym.y + vym.z) / (3.0);
	float fvc = (vc.x + vc.y + vc.z) / (3.0);
	float vx = (fvxp - fvxm) * 0.5;
	float vy = (fvyp - fvym) * 0.5;
	
	// get current position
	float2 pos = asfloat(VbPosition.Load2(vertexID * 8));

	// keep positions on the boundary
	bool onboundary = false;
	if (pos.x < 1.0 / DomainSize.x || pos.x > DomainSize.x - 1.0 / DomainSize.x) {
		onboundary = true;
		vx = 0;
	}
	if (pos.y < 1.0 / DomainSize.y || pos.y > DomainSize.y - 1.0 / DomainSize.y) {
		onboundary = true;
		vy = 0;
	}
	
	float areaslowdown = 1; // maxarea > 0 ? minarea / maxarea : 0;

	float2 dir = -float2(vx, vy) * StepSize;
	float dirl = length(dir);
	if (dirl > TrustRegion)  dir *= TrustRegion / dirl;
	pos += dir * areaslowdown;

	if (UmbrellaDamping > 0)
	{
		float3 center = float3(0, 0, 0);

		// iterate the neighbors
		for (uint poolIndex = entry; poolIndex < entry + count; ++poolIndex)
		{
			// extract triangle index and which corner we are
			int combinedIndex = AdjTriangleLists.Load(poolIndex * 4);
			int triIndex = combinedIndex / 3u;
			int cornerIndex = combinedIndex % 3u;

			// grab the indices of the other two vertices of the triangle
			uint v0 = 0, v1 = 0;
			if (cornerIndex == 0)
			{
				v0 = IndexBuffer.Load((triIndex * 3 + 1) * 4);
				v1 = IndexBuffer.Load((triIndex * 3 + 2) * 4);
			}
			else if (cornerIndex == 1)
			{
				v0 = IndexBuffer.Load((triIndex * 3 + 0) * 4);
				v1 = IndexBuffer.Load((triIndex * 3 + 2) * 4);
			}
			else
			{
				v0 = IndexBuffer.Load((triIndex * 3 + 0) * 4);
				v1 = IndexBuffer.Load((triIndex * 3 + 1) * 4);
			}

			// fetch their positions
			float2 b0 = asfloat(VbPosition.Load2(v0 * 8));
			float2 b1 = asfloat(VbPosition.Load2(v1 * 8));

			center += float3(b0, 1);
			center += float3(b1, 1);
		}
		if (center.z > 0 && !onboundary)
		{
			center /= center.z;
			pos += (center.xy - pos) * /*areaslowdown **/ UmbrellaDamping;
		}
	}

	// clamp after step to boundary
	pos.x = min(max(0, pos.x), DomainSize.x);
	pos.y = min(max(0, pos.y), DomainSize.y);

	// write new result
	VbPosition.Store2(vertexID * 8, asuint(pos));
}
