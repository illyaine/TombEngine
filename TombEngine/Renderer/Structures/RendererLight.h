#pragma once
#include <SimpleMath.h>
#include "Renderer/RendererEnums.h"

namespace TEN::Renderer::Structures
{
	using namespace DirectX::SimpleMath;

	struct RendererLight
	{
		Vector3 Position = Vector3::Zero;
		LightType Type = LightType::HDR;
		Vector3 Color = Vector3::Zero;
		float Intensity = 0.0f;
		Vector3 Direction = Vector3::UnitZ;
		float In = 0.0f;
		float Out = 0.0f;
		float InRange = 0.0f;
		float OutRange = 0.0f;
		
		BoundingSphere BoundingSphere = {};
		int RoomNumber = -1;
		float LocalIntensity = 0.0f;
		float Distance = 0.0f;
		bool AffectNeighbourRooms = false;
		bool CastShadows = false;
		float Luma = 0.0f;

		Vector3 PrevPosition = Vector3::Zero;
		Vector3 PrevDirection = Vector3::UnitZ;

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