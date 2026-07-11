#include "./CBCamera.hlsli"
#include "./CBStarfield.hlsli"
#include "./Math.hlsli"
#include "./ShaderLight.hlsli"
#include "./VertexInput.hlsli"

struct StarfieldInstance
{
	float3 Direction;
	float Scale;
	float3 Color;
	float Extinction;
};

struct WeatherInstance
{
	float3 Position;
	float Size;
	float3 Velocity;
	float Opacity;
	int UniqueID;
	int ClusterSize;
	int FrameIndex;
	int Padding;
};

struct WeatherFrame
{
	float4 UVX;
	float4 UVY;
	int TextureSlice;
	int Padding0;
	int Padding1;
	int Padding2;
};

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD1;
	float4 Color : COLOR;
	float4 FogBulbs : TEXCOORD3;
	float DistanceFog : FOG;
	nointerpolation uint TextureSlice : TEXCOORD4;
};

StructuredBuffer<StarfieldInstance> Stars : register(t14);
StructuredBuffer<WeatherInstance> WeatherParticles : register(t15);
StructuredBuffer<WeatherFrame> WeatherFrames : register(t16);

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);
Texture2D DepthTexture : register(t6);
Texture2DArray WeatherTextureArray : register(t17);

float Hash(uint value)
{
	value ^= value >> 16;
	value *= 0x7feb352du;
	value ^= value >> 15;
	value *= 0x846ca68bu;
	value ^= value >> 16;

	return (value & 0x00ffffffu) / 16777215.0f;
}

float GetTwinkle(uint instanceID)
{
	uint seed = instanceID + 1u;
	seed ^= Frame + 0x9e3779b9u + (seed << 6) + (seed >> 2);

	return lerp(0.5f, 1.0f, Hash(seed));
}

float LegacyAngleToRadians(int angle)
{
	return (angle & 0xffff) * (6.28318530718f / 65536.0f);
}

float3 GetCameraRight()
{
	return normalize(float3(View[0][0], View[1][0], View[2][0]));
}

float3 GetCameraUp()
{
	return normalize(float3(View[0][1], View[1][1], View[2][1]));
}

float GetDepthSeparation(float sceneDepth, float particleDepth)
{
	float depthRange = FarPlane - NearPlane;
	float depthSum = FarPlane + NearPlane;
	float sceneDenominator = depthSum - sceneDepth * depthRange;
	float particleDenominator = depthSum - particleDepth * depthRange;

	// This is algebraically equivalent to LinearizeDepth(sceneDepth) minus
	// LinearizeDepth(particleDepth), but requires only one division.
	return max(
		0.0f,
		(2.0f * NearPlane * depthRange * (sceneDepth - particleDepth)) /
		(sceneDenominator * particleDenominator));
}

void GetWeatherCluster(
	WeatherInstance particle,
	uint clusterIndex,
	out float3 position,
	out float scale,
	out float rotationSine,
	out float rotationCosine)
{
	position = particle.Position;
	scale = particle.Size;
	rotationSine = 0.0f;
	rotationCosine = 1.0f;

	uint uniqueSeed = (uint)particle.UniqueID * 1664525u + clusterIndex * 1013904223u;
	float3 positionOffset = float3(0.0f, 0.0f, 0.0f);

	if (clusterIndex > 0)
	{
		float offsetBase = EnvironmentClusterSpread * 0.8f * ((clusterIndex + 1.0f) / max(1.0f, (float)particle.ClusterSize));
		float xSign = (uniqueSeed & 1u) ? 1.0f : -1.0f;
		float zSign = (uniqueSeed & 4u) ? 1.0f : -1.0f;
		uint axisEmphasis = uniqueSeed & 3u;
		float xScale = axisEmphasis == 0u ? 1.1f : 0.4f;
		float yScale = axisEmphasis == 1u ? 1.2f : 0.5f;
		float zScale = axisEmphasis == 2u ? 1.0f : 0.6f;
		positionOffset = float3(
			xSign * offsetBase * xScale,
			-(offsetBase * yScale),
			zSign * offsetBase * zScale);

		position += positionOffset;
	}

	scale *= lerp(0.75f, 1.35f, Hash(uniqueSeed));

	if (EnvironmentMode == GPU_ENVIRONMENT_SNOW)
	{
		float phase = Hash(uniqueSeed ^ 0x68bc21ebu) * 6.28318530718f + Frame * 0.018f;
		float clusterPhase = (clusterIndex / max(1.0f, (float)particle.ClusterSize)) * 6.28318530718f;
		sincos(phase + clusterPhase, rotationSine, rotationCosine);

		// Reuse the tumble phase for the flutter offsets so snow needs only one trigonometric pair.
		position.x += positionOffset.x * 0.25f * rotationCosine + rotationSine * scale * 0.32f;
		position.z += positionOffset.z * 0.25f * rotationSine + rotationCosine * scale * 0.24f;
		position.y += (rotationSine * rotationCosine) * scale * 0.16f;
	}
}

PixelShaderInput VS(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
	PixelShaderInput output = (PixelShaderInput)0;

	int polyIndex = DecodeIndexInPoly(input.Effects);
	output.TextureSlice = 0;

	float3 worldPosition = float3(0.0f, 0.0f, 0.0f);

	if (EnvironmentMode == GPU_ENVIRONMENT_STARFIELD)
	{
		output.UV = float2(EnvironmentUV[0][polyIndex], EnvironmentUV[1][polyIndex]);

		StarfieldInstance star = Stars[instanceID];
		const float starDistance = 1024.0f;
		const float starSize = 2.0f * star.Scale;
		float3 cameraUp = GetCameraUp();
		float3 billboardForward = star.Direction;
		float3 billboardRight = normalize(cross(cameraUp, billboardForward));
		float3 billboardUp = cross(billboardForward, billboardRight);
		float3 center = CamPositionWS.xyz + star.Direction * starDistance;
		worldPosition = center + billboardRight * input.Position.x * starSize + billboardUp * input.Position.y * starSize;
		output.Color = float4(star.Color, GetTwinkle(instanceID) * star.Extinction);
	}
	else
	{
		uint clusterStride = (uint)max(1, EnvironmentClusterStride);
		uint particleIndex = (uint)EnvironmentParticleOffset + (instanceID / clusterStride);
		uint clusterIndex = instanceID % clusterStride;
		WeatherInstance particle = WeatherParticles[particleIndex];

		// Bucket stride is shared by all particles in a draw. Smaller clusters still leave
		// padded instances, so reject them before hash, trigonometry, billboard and fog work.
		if (clusterIndex >= particle.ClusterSize)
		{
			output.Position = float4(-2.0f, -2.0f, 0.0f, 1.0f);
			return output;
		}

		if (EnvironmentTextureMode == GPU_ENVIRONMENT_TEXTURE_BUCKET)
		{
			output.UV = float2(EnvironmentUV[0][polyIndex], EnvironmentUV[1][polyIndex]);
		}
		else
		{
			WeatherFrame frame = WeatherFrames[particle.FrameIndex];
			output.UV = float2(frame.UVX[polyIndex], frame.UVY[polyIndex]);
			output.TextureSlice = (uint)frame.TextureSlice;
		}

		float3 position;
		float scale;
		float rotationSine;
		float rotationCosine;
		GetWeatherCluster(particle, clusterIndex, position, scale, rotationSine, rotationCosine);

		float3 right;
		float3 up;
		float width = scale;
		float height = scale;
		uint visualSeed = (uint)particle.UniqueID * 747796405u + clusterIndex * 2891336453u;

		if (EnvironmentMode == GPU_ENVIRONMENT_RAIN)
		{
			float velocityLengthSquared = dot(particle.Velocity, particle.Velocity);
			float velocityLength = sqrt(max(velocityLengthSquared, 0.0001f));
			float3 rainAxis = velocityLengthSquared > 0.0001f ? (-particle.Velocity / velocityLength) : float3(0.0f, 1.0f, 0.0f);
			float3 toCameraVector = CamPositionWS.xyz - position;
			float distanceToCamera = sqrt(max(dot(toCameraVector, toCameraVector), 0.0001f));
			float3 toCamera = toCameraVector / distanceToCamera;
			float3 rightCandidate = cross(rainAxis, toCamera);
			if (dot(rightCandidate, rightCandidate) <= 0.0001f)
				rightCandidate = GetCameraRight();

			right = normalize(rightCandidate);
			up = rainAxis;

			const float nearDistance = 512.0f;
			const float farDistance = 5734.4f;
			float widthFactor = saturate((distanceToCamera - nearDistance) / max(1.0f, farDistance - nearDistance));
			width = lerp(1.25f, 10.0f, widthFactor) * lerp(0.8f, 1.15f, Hash(visualSeed));
			height = max(scale * 0.72f, velocityLength * 0.55f) * lerp(0.82f, 1.18f, Hash(visualSeed ^ 0xa511e9b3u));
		}
		else
		{
			right = GetCameraRight();
			up = GetCameraUp();

			if (EnvironmentMode == GPU_ENVIRONMENT_SNOW)
			{
				float3 rotatedRight = right * rotationCosine + up * rotationSine;
				float3 rotatedUp = up * rotationCosine - right * rotationSine;
				right = rotatedRight;
				up = rotatedUp;
				width *= lerp(0.82f, 1.12f, Hash(visualSeed));
				height = width;
			}
		}

		worldPosition = position + right * input.Position.x * width + up * input.Position.y * height;
		float3 color = EnvironmentMode == GPU_ENVIRONMENT_RAIN ? float3(0.82f, 0.91f, 1.0f) : float3(0.96f, 0.985f, 1.0f);
		float opacityVariation = EnvironmentMode == GPU_ENVIRONMENT_RAIN ? lerp(0.72f, 1.0f, Hash(visualSeed ^ 0x63d83595u)) : 1.0f;
		output.Color = float4(color, particle.Opacity * opacityVariation);
	}

	output.Position = mul(float4(worldPosition, 1.0f), ViewProjection);
	output.FogBulbs = DoFogBulbsForVertex(float4(worldPosition, 1.0f));
	output.DistanceFog = DoDistanceFogForVertex(float4(worldPosition, 1.0f));

	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	float particleDepthRaw = 0.0f;
	float sceneDepthRaw = 1.0f;

	if (EnvironmentMode != GPU_ENVIRONMENT_STARFIELD)
	{
		// SV_POSITION already identifies the exact depth-buffer pixel and projected depth.
		particleDepthRaw = input.Position.z;
		sceneDepthRaw = DepthTexture.Load(int3(int2(input.Position.xy), 0)).x;

		// Raw projected depth is sufficient for the occlusion decision and avoids depth
		// linearization for particles that are already hidden behind scene geometry.
		if (particleDepthRaw - sceneDepthRaw > 0.00001f)
			discard;
	}

	// Sample the sprite only after the depth rejection so weather hidden by roofs and
	// walls does not consume color texture bandwidth.
	float4 output;
	if ((EnvironmentMode == GPU_ENVIRONMENT_SNOW || EnvironmentMode == GPU_ENVIRONMENT_RAIN) &&
		EnvironmentTextureMode == GPU_ENVIRONMENT_TEXTURE_ARRAY)
	{
		output = WeatherTextureArray.Sample(Sampler, float3(input.UV, (float)input.TextureSlice)) * input.Color;
	}
	else
	{
		output = Texture.Sample(Sampler, input.UV) * input.Color;
	}

	if (EnvironmentMode != GPU_ENVIRONMENT_STARFIELD)
	{
		float surfaceSeparation = GetDepthSeparation(sceneDepthRaw, particleDepthRaw);

		if (EnvironmentMode == GPU_ENVIRONMENT_UNDERWATER_DUST)
		{
			output.w = min(output.w, surfaceSeparation * 1024.0f);
		}
		else
		{
			output.w *= saturate(surfaceSeparation * 384.0f);
		}
	}

	output.xyz *= 1.0f - Luma(input.FogBulbs.xyz);
	output.xyz = saturate(output.xyz);
	output = DoDistanceFogForPixel(output, float4(0.0f, 0.0f, 0.0f, 0.0f), input.DistanceFog);

	return output;
}
