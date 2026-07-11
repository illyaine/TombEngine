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

	enum class GpuEnvironmentTextureMode : int
	{
		Bucket = 0,
		Atlas = 1,
		Array = 2
	};

	struct alignas(16) CStarfieldBuffer
	{
		Vector4 UV[2];
		GpuEnvironmentMode Mode = GpuEnvironmentMode::Starfield;
		int ClusterStride = 1;
		float ClusterSpread = 0.0f;
		int ParticleOffset = 0;
		GpuEnvironmentTextureMode TextureMode = GpuEnvironmentTextureMode::Bucket;
		int Padding0 = 0;
		int Padding1 = 0;
		int Padding2 = 0;
	};

	static_assert(sizeof(CStarfieldBuffer) == 64);
}
