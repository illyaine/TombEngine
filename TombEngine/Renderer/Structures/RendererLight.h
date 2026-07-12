#pragma once
#include <SimpleMath.h>
#include "Renderer/RendererEnums.h"

namespace TEN::Renderer::Structures
{
	using namespace DirectX::SimpleMath;

	struct RendererLight
	{
		Vector3 Position;
		LightType Type;
		Vector3 Color;
		float Intensity;
		Vector3 Direction;
		float In;
		float Out;
		float InRange;
		float OutRange;
		
		BoundingSphere BoundingSphere;
		int RoomNumber;
		float LocalIntensity;
		float Distance;
		bool AffectNeighbourRooms;
		bool CastShadows;
		float Luma;

		Vector3 PrevPosition;
		Vector3 PrevDirection;

		int Hash = 0;

		float CachedSpotInRange = 0.0f;
		float CachedSpotOutRange = 0.0f;
		float CachedSpotInRangeCos = 0.0f;
		float CachedSpotOutRangeCos = 0.0f;
		bool SpotRangeCacheValid = false;

		Vector3 CachedInterpolatedPosition = Vector3::Zero;
		Vector3 CachedInterpolatedDirection = Vector3::Zero;
		float CachedInterpolationFactor = 0.0f;
		bool InterpolationCacheValid = false;
	};

	struct RendererLightNode
	{
		RendererLight* Light;
		float LocalIntensity;
		float Distance;
		int Dynamic;
	};
}