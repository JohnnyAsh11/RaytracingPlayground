#include "Random.hlsli"

// === Structs ===
struct Vertex
{
    float3 localPosition;
    float2 uv;
    float3 normal;
    float3 tangent;
};
struct RayPayload
{
    float3 color;
    unsigned int RecursionDepth;
    unsigned int RayPerPixelIndex;
};

// === Constant buffers ===
struct SceneData
{
	matrix InverseViewProjection;
    float3 CameraPosition;
    uint RaysPerPixel;
    uint RecursionDepth;
};
struct EntityData
{
    float4 Color;
	
    uint VertexBufferDescriptorIndex;
    uint IndexBufferDescriptorIndex;
	uint AlbedoIndex;
	uint NormalIndex;
	
	uint RoughnessIndex;
    uint MetalnessIndex;
    uint EmissiveIndex;
    float Roughness;
	
    float Metalness;
    float3 padding;
};

cbuffer DrawData : register(b0)
{
    uint SceneDataConstantBufferIndex;
    uint EntityDataDescriptorIndex;
    uint SceneTLASDescriptorIndex;
    uint OutputUAVDescriptorIndex;
    uint SkyboxDescriptorIndex;
};

SamplerState Sampler : register(s0);

float FresnelView(float3 n, float3 v, float f0)
{
    return f0 + (1 - f0) * pow(1 - saturate(dot(n, v)), 5);
}

// === Helpers ===
// Barycentric interpolation of data from the triangle's vertices
Vertex InterpolateVertices(uint triangleIndex, float2 barycentrics)
{	
	// Get the data for this entity
    StructuredBuffer<EntityData> ed =
		ResourceDescriptorHeap[EntityDataDescriptorIndex];
    EntityData thisEntity = ed[InstanceIndex()];
	
	// Get the geometry buffers
    StructuredBuffer<uint> IndexBuffer =
		ResourceDescriptorHeap[thisEntity.IndexBufferDescriptorIndex];
    StructuredBuffer<Vertex> VertexBuffer =
		ResourceDescriptorHeap[thisEntity.VertexBufferDescriptorIndex];
	
	// Grab the 3 indices for this triangle
	uint firstIndex = triangleIndex * 3;
	uint indices[3];
	indices[0] = IndexBuffer[firstIndex + 0];
	indices[1] = IndexBuffer[firstIndex + 1];
	indices[2] = IndexBuffer[firstIndex + 2];

	// Grab the 3 corresponding vertices
	Vertex verts[3];
	verts[0] = VertexBuffer[indices[0]];
	verts[1] = VertexBuffer[indices[1]];
	verts[2] = VertexBuffer[indices[2]];
	
	// Calculate the barycentric data for vertex interpolation
	float3 barycentricData = float3(
		1.0f - barycentrics.x - barycentrics.y,
		barycentrics.x,
		barycentrics.y);
	
	// Loop through the barycentric data and interpolate
    Vertex finalVert = (Vertex) 0;
	
	for (uint i = 0; i < 3; i++)
	{
		finalVert.localPosition += verts[i].localPosition * barycentricData[i];
		finalVert.uv += verts[i].uv * barycentricData[i];
		finalVert.normal += verts[i].normal * barycentricData[i];
		finalVert.tangent += verts[i].tangent * barycentricData[i];
	}
	return finalVert;
}

// Calculates an origin and direction from the camera for specific pixel indices
RayDesc CalcRayFromCamera(float2 rayIndices, float3 camPos, float4x4 invVP)
{
	// Offset to the middle of the pixel
	float2 pixel = rayIndices + 0.5f;
	float2 screenPos = pixel / DispatchRaysDimensions().xy * 2.0f - 1.0f;
	screenPos.y = -screenPos.y;

	// Unproject the coords
	float4 worldPos = mul(invVP, float4(screenPos, 0, 1));
	worldPos.xyz /= worldPos.w;

	// Set up the ray
	RayDesc ray;
	ray.Origin = camPos.xyz;
	ray.Direction = normalize(worldPos.xyz - ray.Origin);
	ray.TMin = 0.01f;
	ray.TMax = 1000.0f;
	return ray;
}

// === Shaders ===
// Ray generation shader - Launched once for each ray we want to generate
// (which is generally once per pixel of our output texture)
[shader("raygeneration")]
void RayGen()
{	
	// Grabbing the constant buffer and TLAS.
    ConstantBuffer<SceneData> cb =
		ResourceDescriptorHeap[SceneDataConstantBufferIndex];
    RaytracingAccelerationStructure TLAS = 
		ResourceDescriptorHeap[SceneTLASDescriptorIndex];
	
	// Getting the ray indices.
    uint2 rayIndices = DispatchRaysIndex().xy;

    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < cb.RaysPerPixel; i++)
    {
        float2 currentIndices = rayIndices;
        float ray = (float) i / cb.RaysPerPixel;
        currentIndices += rand2(rayIndices * ray);
		
		// Calculate the ray for this pixel and iteration.
        RayDesc rayDesc = CalcRayFromCamera(
			rayIndices,
			cb.CameraPosition,
			cb.InverseViewProjection);
		
		// Creating the payload for this ray.
        RayPayload payload = (RayPayload) 0;
        payload.color = float3(1.0f, 1.0f, 1.0f);
        payload.RecursionDepth = 0;
		
		// Setting the index according the current iteration.
		payload.RayPerPixelIndex = i;
		
		// Tracing the ray and accumulating the color.
        TraceRay(
			TLAS,
			RAY_FLAG_NONE,
			0xFF,
			0, 0, 0,
			rayDesc,
			payload);
        finalColor += payload.color;
    }
	
	// Set the final color of the buffer
    RWTexture2D<float4> OutputColor = ResourceDescriptorHeap[OutputUAVDescriptorIndex];
    OutputColor[rayIndices] = float4(pow(finalColor / cb.RaysPerPixel, 1.0f / 2.2f), 1.0f);
}


// Miss shader - What happens if the ray doesn't hit anything?
[shader("miss")]
void Miss(inout RayPayload payload)
{
    if (SkyboxDescriptorIndex != -1)
    {
		// Grabbing the cubemap resource.
        TextureCube skybox = ResourceDescriptorHeap[SkyboxDescriptorIndex];
		
		// Sampling the cubemap at the direction of the ray in world space.
        payload.color *= skybox.SampleLevel(Sampler, WorldRayDirection(), 0).rgb;
    }
}


// Closest hit shader - Runs the first time a ray hits anything
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
    ConstantBuffer<SceneData> cb =
		ResourceDescriptorHeap[SceneDataConstantBufferIndex];
	
    if (payload.RecursionDepth == cb.RecursionDepth)
    {
        payload.color = float3(0.0f, 0.0f, 0.0f);
        return;
    }
	
	// Get the data for this entity
    StructuredBuffer<EntityData> entityDataBuffer =
		ResourceDescriptorHeap[EntityDataDescriptorIndex];
    EntityData thisEntity = entityDataBuffer[InstanceIndex()];
	
	// Grab the geometry and convert the normal to world space.
    Vertex currentVertex = InterpolateVertices(PrimitiveIndex(), hitAttributes.barycentrics);
    float3x3 objectToWorld = (float3x3) ObjectToWorld4x3();
    float3 normal = normalize(mul(currentVertex.normal, objectToWorld));
    float tangent = normalize(mul(currentVertex.tangent, objectToWorld));
	
	// Setting basic material values.
    float metalness = thisEntity.Metalness;
    float roughness = saturate(pow(thisEntity.Roughness, 2.0f));
    float3 baseColor = thisEntity.Color.rgb;
	
	
	// Essentially translates to "If there is a texture".
    if (thisEntity.AlbedoIndex != -1)
    {
        Texture2D Albedo = ResourceDescriptorHeap[thisEntity.AlbedoIndex];
        Texture2D Normal = ResourceDescriptorHeap[thisEntity.NormalIndex];
        Texture2D Rough = ResourceDescriptorHeap[thisEntity.RoughnessIndex];
        Texture2D Metal = ResourceDescriptorHeap[thisEntity.MetalnessIndex];
		
        baseColor = pow(Albedo.SampleLevel(Sampler, currentVertex.uv, 0).rgb, 2.2f);
        roughness = pow(Rough.SampleLevel(Sampler, currentVertex.uv, 0).r, 2);
        metalness = Metal.SampleLevel(Sampler, currentVertex.uv, 0).r;
        float3 normalMap = Normal.SampleLevel(Sampler, currentVertex.uv, 0).rgb * 2 - 1;
		
        float3 N = normal;
        float3 T = normalize(tangent - N * dot(tangent, N));
        float3 B = cross(T, N);
        float3x3 TBN = float3x3(T, B, N);
        normal = normalize(mul(normalMap, TBN));
    }
	
	// Calc a unique RNG value for this ray, based on the "uv" (0-1 location) of this pixel and other per-ray data
    float2 pixelUV = (float2) DispatchRaysIndex().xy / DispatchRaysDimensions().xy;
    float2 rng = rand2(pixelUV * (payload.RecursionDepth + 1) + payload.RayPerPixelIndex + RayTCurrent());
	
	// Interpolate between perfect reflection and random bounce based on roughness
    float3 refl = reflect(WorldRayDirection(), normal);
    float3 randomBounce = RandomCosineWeightedHemisphere(rand(rng), rand(rng.yx), normal);
    float3 dir = normalize(lerp(refl, randomBounce, roughness));
	
	// Interpolate between fully random bounce and roughness-based bounce based on fresnel/metal switch
	// - If we're a "diffuse" ray, we need a random bounce
	// - If we're a "specular" ray, we need the roughness-based bounce
	// - Metals will have a fresnel result of 1.0, so this won't affect them
    float fres = FresnelView(-WorldRayDirection(), normal, lerp(0.04f, 1.0f, metalness));
    dir = normalize(lerp(randomBounce, dir, fres > rng.x));
	
	// Determine how we color the ray:
	// - If this is a "diffuse" ray, use the surface color
	// - If this is a "specular" ray, assume a bounce without tint
	// - Metals always tint, so the final lerp below takes care of that
    float3 roughnessBounceColor = lerp(float3(1, 1, 1), baseColor, roughness); // Dir is roughness-based, so color is too
    float3 diffuseColor = lerp(baseColor, roughnessBounceColor, fres > rng.x); // Diffuse "reflection" chance
    float3 finalColor = lerp(diffuseColor, baseColor, metalness); // Metal always tints
		
    payload.color *= finalColor;
    if (thisEntity.EmissiveIndex != -1)
    {
        Texture2D Emissive = ResourceDescriptorHeap[thisEntity.EmissiveIndex];
        float3 emissiveColor = pow(Emissive.SampleLevel(Sampler, currentVertex.uv, 0.0f).rgb, 2.2f);
		
        payload.color += payload.color * emissiveColor * 40.0f;
    }
	
	// Create the new recursive ray
    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = dir;
    ray.TMin = 0.0001f;
    ray.TMax = 1000.0f;
	
	// Recursive ray trace
    payload.RecursionDepth++;
    RaytracingAccelerationStructure TLAS = ResourceDescriptorHeap[SceneTLASDescriptorIndex];
    TraceRay(
		TLAS,
		RAY_FLAG_NONE,
		0xFF, 0, 0, 0, // Mask and offsets
		ray,
		payload);
}