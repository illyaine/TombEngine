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
	int Padding0;
	int Padding1;
};

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD1;
	float4 Color : COLOR;
	float4 PositionCopy : TEXCOORD2;
	float4 FogBulbs : TEXCOORD3;
	float DistanceFog : FOG;
	float Active : TEXCOORD4;
};

StructuredBuffer<StarfieldInstance> Stars : register(t14);
StructuredBuffer<WeatherInstance> WeatherParticles : register(t15);

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);
Texture2D DepthTexture : register(t6);
SamplerState DepthSampler : register(s6);

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

void GetWeatherCluster(
	WeatherInstance particle,
	uint clusterIndex,
	out float3 position,
	out float scale,
	out float rotation)
{
	position = particle.Position;
	scale = particle.Size;
	rotation = 0.0f;

	if (clusterIndex == 0)
		return;

	int uniqueSeed = particle.UniqueID + clusterIndex;
	float offsetBase = EnvironmentClusterSpread * ((clusterIndex + 1.0f) / particle.ClusterSize);
	float xSign = (uniqueSeed & 1) ? 1.0f : -1.0f;
	float zSign = (uniqueSeed & 4) ? 1.0f : -1.0f;
	int axisEmphasis = uniqueSeed & 3;
	float xScale = axisEmphasis == 0 ? 1.1f : 0.4f;
	float yScale = axisEmphasis == 1 ? 1.2f : 0.5f;
	float zScale = axisEmphasis == 2 ? 1.0f : 0.6f;
	float3 positionOffset = float3(
		xSign * offsetBase * xScale,
		-(offsetBase * yScale),
		zSign * offsetBase * zScale);

	position += positionOffset;
	scale *= 1.0f + abs(sin(LegacyAngleToRadians(uniqueSeed)));

	if (EnvironmentMode == GPU_ENVIRONMENT_SNOW)
	{
		int spinAngle = (int(abs(position.y)) % 3072) * 21;
		float spin = LegacyAngleToRadians(spinAngle);
		position.x += positionOffset.x * sin(spin);
		position.z += positionOffset.z * cos(spin);
		rotation = (clusterIndex / (float)particle.ClusterSize) * 6.28318530718f;
	}
}

PixelShaderInput VS(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
	PixelShaderInput output = (PixelShaderInput)0;
	output.Active = 1.0f;

	int polyIndex = DecodeIndexInPoly(input.Effects);
	output.UV = float2(EnvironmentUV[0][polyIndex], EnvironmentUV[1][polyIndex]);

	float3 worldPosition = float3(0.0f, 0.0f, 0.0f);

	if (EnvironmentMode == GPU_ENVIRONMENT_STARFIELD)
	{
		StarfieldInstance star = Stars[instanceID];
		const float starDistance = 1024.0f;
		const float starSize = 2.0f * star.Scale;
		float3 cameraUp = GetCameraUp();
		float3 billboardForward = normalize(star.Direction);
		float3 billboardRight = normalize(cross(cameraUp, billboardForward));
		float3 billboardUp = cross(billboardForward, billboardRight);
		float3 center = CamPositionWS.xyz + star.Direction * starDistance;
		worldPosition = center + billboardRight * input.Position.x * starSize + billboardUp * input.Position.y * starSize;
		output.Color = float4(star.Color, GetTwinkle(instanceID) * star.Extinction);
	}
	else
	{
		uint clusterStride = (uint)max(1, EnvironmentClusterStride);
		uint particleIndex = instanceID / clusterStride;
		uint clusterIndex = instanceID % clusterStride;
		WeatherInstance particle = WeatherParticles[particleIndex];

		if (clusterIndex >= particle.ClusterSize)
			output.Active = 0.0f;

		float3 position;
		float scale;
		float rotation;
		GetWeatherCluster(particle, clusterIndex, position, scale, rotation);

		float3 right;
		float3 up;
		float width = scale;
		float height = scale;

		if (EnvironmentMode == GPU_ENVIRONMENT_RAIN)
		{
			float velocityLengthSquared = dot(particle.Velocity, particle.Velocity);
			float3 rainAxis = velocityLengthSquared > 0.0001f ? normalize(-particle.Velocity) : float3(0.0f, 1.0f, 0.0f);
			float3 toCamera = normalize(CamPositionWS.xyz - position);
			float3 rightCandidate = cross(rainAxis, toCamera);
			if (dot(rightCandidate, rightCandidate) <= 0.0001f)
				rightCandidate = GetCameraRight();

			right = normalize(rightCandidate);
			up = rainAxis;

			const float nearDistance = 512.0f;
			const float farDistance = 5734.4f;
			float distanceToCamera = length(position - CamPositionWS.xyz);
			float widthFactor = saturate((distanceToCamera - nearDistance) / max(1.0f, farDistance - nearDistance));
			width = lerp(1.5f, 15.0f, widthFactor);
		}
		else
		{
			right = GetCameraRight();
			up = GetCameraUp();

			if (EnvironmentMode == GPU_ENVIRONMENT_SNOW)
			{
				float sine = sin(rotation);
				float cosine = cos(rotation);
				float3 rotatedRight = right * cosine + up * sine;
				float3 rotatedUp = up * cosine - right * sine;
				right = rotatedRight;
				up = rotatedUp;
			}
		}

		worldPosition = position + right * input.Position.x * width + up * input.Position.y * height;
		float3 color = EnvironmentMode == GPU_ENVIRONMENT_RAIN ? float3(0.8f, 1.0f, 1.0f) : float3(1.0f, 1.0f, 1.0f);
		output.Color = float4(color, particle.Opacity);
	}

	output.Position = mul(float4(worldPosition, 1.0f), ViewProjection);
	output.PositionCopy = output.Position;
	output.FogBulbs = DoFogBulbsForVertex(float4(worldPosition, 1.0f));
	output.DistanceFog = DoDistanceFogForVertex(float4(worldPosition, 1.0f));

	return output;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
	clip(input.Active - 0.5f);

	float4 output = Texture.Sample(Sampler, input.UV) * input.Color;

	if (EnvironmentMode == GPU_ENVIRONMENT_UNDERWATER_DUST)
	{
		float particleDepth = input.PositionCopy.z / input.PositionCopy.w;
		input.PositionCopy.xy /= input.PositionCopy.w;
		float2 texCoord = 0.5f * (float2(input.PositionCopy.x, -input.PositionCopy.y) + 1.0f);
		float sceneDepth = DepthTexture.Sample(DepthSampler, texCoord).x;
		sceneDepth = LinearizeDepth(sceneDepth, NearPlane, FarPlane);
		particleDepth = LinearizeDepth(particleDepth, NearPlane, FarPlane);

		if (particleDepth - sceneDepth > 0.01f)
			discard;

		float fade = (sceneDepth - particleDepth) * 1024.0f;
		output.w = min(output.w, fade);
	}

	output.xyz *= 1.0f - Luma(input.FogBulbs.xyz);
	output.xyz = saturate(output.xyz);
	output = DoDistanceFogForPixel(output, float4(0.0f, 0.0f, 0.0f, 0.0f), input.DistanceFog);

	return output;
}
