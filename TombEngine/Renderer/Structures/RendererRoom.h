#pragma once

#include <vector>
#include <SimpleMath.h>

#include "Renderer/Graphics/RenderTarget2D.h"
#include "Renderer/Structures/RendererBucket.h"
#include "Renderer/Structures/RendererDecal.h"
#include "Renderer/Structures/RendererDoor.h"
#include "Renderer/Structures/RendererEffect.h"
#include "Renderer/Structures/RendererItem.h"
#include "Renderer/Structures/RendererLight.h"
#include "Renderer/Structures/RendererRectangle.h"
#include "Renderer/Structures/RendererStatic.h"

namespace TEN::Renderer::Structures
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	using namespace TEN::Renderer::Graphics;

	struct RendererRoomLightCandidate
	{
		RendererLight* Light = nullptr;
		int SourceRoomNumber = -1;
	};

	struct RendererRoom
	{
		bool Visited;
		short RoomNumber;
		Vector4 AmbientLight;
		Vector4 ViewPort;
		std::vector<RendererBucket> Buckets;
		std::vector<RendererLight> Lights;
		std::vector<RendererStatic> Statics;
		std::vector<RendererItem*> ItemsToDraw;
		std::vector<RendererEffect*> EffectsToDraw;
		std::vector<RendererStatic*> StaticsToDraw;
		std::vector<RendererLight*> LightsToDraw;
		std::vector<RendererRoomLightCandidate> StaticLightCandidates;
		std::vector<RendererLight*> DynamicLightCandidates;
		std::vector<RendererDecal> Decals;
		std::vector<RendererDoor> Doors;
		BoundingBox BoundingBox;
		RendererRectangle ClipBounds;
		std::vector<int> Neighbors;
		unsigned int VisibilityGeneration = 0;
		unsigned int FogCollectionGeneration = 0;
		bool StaticLightCandidatesValid = false;
		bool DynamicLightCandidatesReady = false;
	};
}