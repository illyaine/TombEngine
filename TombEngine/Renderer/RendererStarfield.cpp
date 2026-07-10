#include "framework.h"
#include "Renderer/Renderer.h"

#include <chrono>

#include "Game/effects/weather.h"
#include "Game/Setup.h"
#include "Objects/game_object_ids.h"

using namespace TEN::Effects::Environment;
using namespace TEN::Renderer::ConstantBuffers;
using namespace TEN::Renderer::Structures;

namespace TEN::Renderer
{
	constexpr auto STARFIELD_BUFFER_SLOT = 14;

	void Renderer::UpdateStarfieldBuffer()
	{
		const auto revision = Weather.GetStarfieldRevision();
		if (_starfieldRevision == revision)
			return;

		const auto preparationStart = std::chrono::high_resolution_clock::now();

		_starfieldRevision = revision;
		_starfieldCount = 0;
		_starfieldBuffer.Reset();
		_starfieldBufferView.Reset();

		const auto& stars = Weather.GetStars();
		if (stars.empty())
			return;

		auto* sprite = &_sprites[Objects[ID_DEFAULT_SPRITES].meshIndex + SPR_LENS_FLARE_3];

		// NOTE: Strange packing due to particular HLSL 16-byte alignment requirements.
		_stStarfield.UV[0].x = sprite->UV[0].x;
		_stStarfield.UV[0].y = sprite->UV[1].x;
		_stStarfield.UV[0].z = sprite->UV[2].x;
		_stStarfield.UV[0].w = sprite->UV[3].x;
		_stStarfield.UV[1].x = sprite->UV[0].y;
		_stStarfield.UV[1].y = sprite->UV[1].y;
		_stStarfield.UV[1].z = sprite->UV[2].y;
		_stStarfield.UV[1].w = sprite->UV[3].y;

		auto rendererStars = std::vector<RendererStar>{};
		rendererStars.reserve(stars.size());

		for (const auto& star : stars)
		{
			rendererStars.push_back(
			{
				star.Direction,
				star.Scale,
				star.Color,
				star.Extinction
			});
		}

		auto bufferDesc = D3D11_BUFFER_DESC{};
		bufferDesc.ByteWidth = static_cast<UINT>(sizeof(RendererStar) * rendererStars.size());
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = sizeof(RendererStar);

		auto initialData = D3D11_SUBRESOURCE_DATA{};
		initialData.pSysMem = rendererStars.data();

		Utils::throwIfFailed(_device->CreateBuffer(&bufferDesc, &initialData, _starfieldBuffer.GetAddressOf()));

		auto viewDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = static_cast<UINT>(rendererStars.size());

		Utils::throwIfFailed(_device->CreateShaderResourceView(_starfieldBuffer.Get(), &viewDesc, _starfieldBufferView.GetAddressOf()));
		_starfieldCount = static_cast<int>(rendererStars.size());

		const auto preparationEnd = std::chrono::high_resolution_clock::now();
		const auto preparationTime = std::chrono::duration_cast<std::chrono::microseconds>(preparationEnd - preparationStart).count();
		TENLog(
			"Prepared GPU starfield buffer with " + std::to_string(_starfieldCount) +
			" stars in " + std::to_string(preparationTime) + " microseconds.",
			LogLevel::Info, LogConfig::Debug);
	}

	void Renderer::DrawStarfield()
	{
		UpdateStarfieldBuffer();

		if (_starfieldCount == 0 || _starfieldBufferView == nullptr)
			return;

		_stStarfield.Mode = GpuEnvironmentMode::Starfield;
		_stStarfield.ClusterStride = 1;
		_stStarfield.ClusterSpread = 0.0f;
		UpdateConstantBuffer(_stStarfield, _cbStarfield);
		BindConstantBufferVS(ConstantBufferRegister::InstancedSprites, _cbStarfield.get());
		BindConstantBufferPS(ConstantBufferRegister::InstancedSprites, _cbStarfield.get());

		SetDepthState(DepthState::Read);
		SetBlendMode(BlendMode::Additive);
		SetCullMode(CullMode::None);

		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		_context->IASetInputLayout(_inputLayout.Get());

		unsigned int stride = sizeof(Vertex);
		unsigned int offset = 0;
		_context->IASetVertexBuffers(0, 1, _quadVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);

		_shaders.Bind(Shader::Starfield);
		auto* sprite = &_sprites[Objects[ID_DEFAULT_SPRITES].meshIndex + SPR_LENS_FLARE_3];
		BindTexture(TextureRegister::ColorMap, sprite->Texture, SamplerStateRegister::LinearClamp);

		auto* starfieldView = _starfieldBufferView.Get();
		_context->VSSetShaderResources(STARFIELD_BUFFER_SLOT, 1, &starfieldView);

		DrawInstancedTriangles(4, _starfieldCount, 0);

		ID3D11ShaderResourceView* nullView = nullptr;
		_context->VSSetShaderResources(STARFIELD_BUFFER_SLOT, 1, &nullView);
		BindConstantBufferVS(ConstantBufferRegister::InstancedSprites, _cbInstancedSpriteBuffer.get());
		BindConstantBufferPS(ConstantBufferRegister::InstancedSprites, _cbInstancedSpriteBuffer.get());

		_numInstancedSpritesDrawCalls++;
	}
}
