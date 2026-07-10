#pragma once
#include <SimpleMath.h>

namespace TEN::Renderer::Structures
{
	using namespace DirectX::SimpleMath;

	struct alignas(16) RendererStar
	{
		Vector3 Direction;
		float Scale;
		Vector3 Color;
		float Extinction;
	};

	static_assert(sizeof(RendererStar) == 32);
}
