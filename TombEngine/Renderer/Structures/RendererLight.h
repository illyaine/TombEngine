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
	};

	struct RendererLightNode
	{
		RendererLight* Light = nullptr;
		float LocalIntensity = 0.0f;
		float Distance = 0.0f;
		int Dynamic = 0;

		RendererLightNode() = default;

		RendererLightNode(RendererLight* light, float localIntensity, float distance, int dynamic)
			: Light(light), LocalIntensity(localIntensity), Distance(distance), Dynamic(dynamic)
		{
			// Object shaders have a limited direct-light array. Directional lights
			// represent scene-wide sunlight or moonlight and must not disappear when
			// several nearby point or spot lights compete for those slots.
			if (Light != nullptr && Light->Type == LightType::Sun)
				LocalIntensity += 1000000.0f;
		}
	};
}