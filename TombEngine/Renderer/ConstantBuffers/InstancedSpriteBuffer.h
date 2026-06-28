#pragma once
#include <SimpleMath.h>
#include "Renderer/RendererEnums.h"

namespace TEN::Renderer::ConstantBuffers
{
	using namespace DirectX::SimpleMath;

	struct alignas(16) InstancedSprite
	{
		Matrix World;
		Vector4 UV[2];
		Vector4 Color;
		float IsBillboard;
		float IsSoftParticle;
		int RenderType;
		int PerVertexColor;
	};

	struct alignas(16) CInstancedSpriteBuffer
	{
		InstancedSprite Sprites[INSTANCED_SPRITES_BUCKET_SIZE];
	};

	static_assert(sizeof(InstancedSprite) == 128,
		"Instanced sprite GPU data must stay at 128 bytes to preserve the 512-instance D3D11 constant-buffer limit.");
	static_assert(sizeof(CInstancedSpriteBuffer) == 65536,
		"Instanced sprite constant buffer must not exceed the D3D11 64 KiB limit.");
}
