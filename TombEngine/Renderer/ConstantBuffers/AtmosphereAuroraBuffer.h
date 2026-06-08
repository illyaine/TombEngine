#pragma once
#include <SimpleMath.h>

namespace TEN::Renderer::ConstantBuffers
{
	using namespace DirectX::SimpleMath;

	struct alignas(16) CAtmosphereAuroraBuffer
	{
		Vector4 ColorA;
		Vector4 ColorB;
		Vector4 ColorC;
		Vector4 Controls;
		Vector4 Waves;
		Vector4 Time;
	};
}
