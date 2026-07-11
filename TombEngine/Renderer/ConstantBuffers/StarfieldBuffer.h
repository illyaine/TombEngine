#pragma once
#include <SimpleMath.h>

namespace TEN::Renderer::ConstantBuffers
{
	using namespace DirectX::SimpleMath;

	enum class GpuEnvironmentMode : int
	{
		Starfield = 0,
		UnderwaterDust = 1,
		Snow = 2,
		Rain = 3
	};

	struct alignas(16) CStarfieldBuffer
	{
		Vector4 UV[2];
		GpuEnvironmentMode Mode = GpuEnvironmentMode::Starfield;
		int ClusterStride = 1;
		float ClusterSpread = 0.0f;
		int ParticleOffset = 0;
	};

	static_assert(sizeof(CStarfieldBuffer) == 48);
}
