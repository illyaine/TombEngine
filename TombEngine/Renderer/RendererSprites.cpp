#include "framework.h"
#include "Renderer/Structures/RendererSprite.h"

#include "Game/effects/weather.h"
#include "Game/Setup.h"
#include "Renderer/Graphics/Texture2DArray.h"
#include "Renderer/Structures/RendererSpriteBucket.h"
#include "Renderer/Renderer.h"
#include "Specific/Parallel.h"

using namespace TEN::Effects::Environment;
using namespace TEN::Renderer::ConstantBuffers;
using namespace TEN::Renderer::Graphics;
using namespace TEN::Renderer::Structures;

namespace TEN::Renderer
{
	namespace
	{
		constexpr auto WEATHER_BUFFER_SLOT = 15;
		constexpr auto WEATHER_FRAME_BUFFER_SLOT = 16;
		constexpr auto WEATHER_TEXTURE_ARRAY_SLOT = 17;
		constexpr auto GPU_WEATHER_CLUSTER_STRIDE = 16;

		struct alignas(16) RendererWeatherParticle
		{
			Vector3 Position = Vector3::Zero;
			float Size = 0.0f;
			Vector3 Velocity = Vector3::Zero;
			float Opacity = 0.0f;
			int UniqueID = 0;
			int ClusterSize = 1;
			int FrameIndex = 0;
			int Padding = 0;
		};

		static_assert(sizeof(RendererWeatherParticle) == 48);

		struct alignas(16) RendererWeatherFrame
		{
			Vector4 UVX = Vector4::Zero;
			Vector4 UVY = Vector4::Zero;
			int TextureSlice = 0;
			int Padding0 = 0;
			int Padding1 = 0;
			int Padding2 = 0;
		};

		static_assert(sizeof(RendererWeatherFrame) == 48);

		struct WeatherGpuBuffer
		{
			ComPtr<ID3D11Buffer> Buffer = nullptr;
			ComPtr<ID3D11ShaderResourceView> View = nullptr;
			size_t Capacity = 0;
		};

		struct WeatherCpuBucket
		{
			RendererSprite* Sprite = nullptr;
			std::vector<RendererWeatherParticle> Particles = {};
			int MaxClusterSize = 1;
			int ParticleOffset = 0;
		};

		struct WeatherFrameSource
		{
			RendererSprite* Sprite = nullptr;
			Texture2D* Texture = nullptr;
			ID3D11Texture2D* Resource = nullptr;
			Vector2 UV[4] = {};
		};

		struct WeatherTextureCache
		{
			bool CanBatch = false;
			bool UsesTextureArray = false;
			Texture2D* AtlasTexture = nullptr;
			Texture2DArray TextureArray = {};
			ComPtr<ID3D11Buffer> FrameBuffer = nullptr;
			ComPtr<ID3D11ShaderResourceView> FrameView = nullptr;
			std::vector<WeatherFrameSource> Sources = {};
		};
	}

	void Renderer::AddSpriteBillboard(RendererSprite* sprite, const Vector3& pos, const Vector4& color, float orient2D, float scale,
									  Vector2 size, BlendMode blendMode, bool isSoftParticle, RenderView& view, SpriteRenderType renderType)
	{
		if (scale <= 0.0f)
			scale = 1.0f;

		size.x *= scale;
		size.y *= scale;

		RendererSpriteToDraw spr = {};

		spr.Type = SpriteType::Billboard;
		spr.Sprite = sprite;
		spr.pos = pos;
		spr.Rotation = orient2D;
		spr.Scale = scale;
		spr.Width = size.x;
		spr.Height = size.y;
		spr.BlendMode = blendMode;
		spr.SoftParticle = isSoftParticle;
		spr.c1 = color;
		spr.c2 = color;
		spr.c3 = color;
		spr.c4 = color;
		spr.color = color;
		spr.renderType = renderType;

		view.SpritesToDraw.push_back(spr);
	}

	void Renderer::AddSpriteBillboardConstrained(RendererSprite* sprite, const Vector3& pos, const Vector4& color, float orient2D,
										 float scale, Vector2 size, BlendMode blendMode, const Vector3& constrainAxis,
										 bool isSoftParticle, RenderView& view, SpriteRenderType renderType)
	{
		if (scale <= 0.0f)
			scale = 1.0f;

		size.x *= scale;
		size.y *= scale;

		RendererSpriteToDraw spr = {};

		spr.Type = SpriteType::CustomBillboard;
		spr.Sprite = sprite;
		spr.pos = pos;
		spr.Rotation = orient2D;
		spr.Scale = scale;
		spr.Width = size.x;
		spr.Height = size.y;
		spr.BlendMode = blendMode;
		spr.ConstrainAxis = constrainAxis;
		spr.SoftParticle = isSoftParticle;
		spr.c1 = color;
		spr.c2 = color;
		spr.c3 = color;
		spr.c4 = color;
		spr.color = color;
		spr.renderType = renderType;

		view.SpritesToDraw.push_back(spr);
	}

	void Renderer::AddSpriteBillboardConstrainedLookAt(RendererSprite* sprite, const Vector3& pos, const Vector4& color, float orient2D,
		float scale, Vector2 size, BlendMode blendMode, const Vector3& lookAtAxis,
		bool isSoftParticle, RenderView& view, SpriteRenderType renderType)
	{
		if (scale <= 0.0f)
			scale = 1.0f;

		size.x *= scale;
		size.y *= scale;

		RendererSpriteToDraw spr = {};

		spr.Type = SpriteType::LookAtBillboard;
		spr.Sprite = sprite;
		spr.pos = pos;
		spr.Rotation = orient2D;
		spr.Scale = scale;
		spr.Width = size.x;
		spr.Height = size.y;
		spr.BlendMode = blendMode;
		spr.LookAtAxis = lookAtAxis;
		spr.SoftParticle = isSoftParticle;
		spr.c1 = color;
		spr.c2 = color;
		spr.c3 = color;
		spr.c4 = color;
		spr.color = color;
		spr.renderType = renderType;

		view.SpritesToDraw.push_back(spr);
	}

	void Renderer::AddQuad(RendererSprite* sprite, const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
		const Vector4 color, float orient2D, float scale, Vector2 size, BlendMode blendMode, bool softParticles,
		RenderView& view)
	{
		AddQuad(sprite, vertex0, vertex1, vertex2, vertex3, color, color, color, color, orient2D, scale, size, blendMode, softParticles, view, SpriteRenderType::Default);
	}

	void Renderer::AddQuad(RendererSprite* sprite, const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
		const Vector4& color0, const Vector4& color1, const Vector4& color2, const Vector4& color3, float orient2D,
		float scale, Vector2 size, BlendMode blendMode, bool isSoftParticle, RenderView& view, SpriteRenderType renderType)
	{
		if (scale <= 0.0f)
			scale = 1.0f;

		size.x *= scale;
		size.y *= scale;

		RendererSpriteToDraw spr = {};

		spr.Type = SpriteType::ThreeD;
		spr.Sprite = sprite;
		spr.vtx1 = vertex0;
		spr.vtx2 = vertex1;
		spr.vtx3 = vertex2;
		spr.vtx4 = vertex3;
		spr.c1 = color0;
		spr.c2 = color1;
		spr.c3 = color2;
		spr.c4 = color3;
		spr.Rotation = orient2D;
		spr.Scale = scale;
		spr.Width = size.x;
		spr.Height = size.y;
		spr.BlendMode = blendMode;
		spr.pos = (vertex0 + vertex1 + vertex2 + vertex3) / 4.0f;
		spr.SoftParticle = isSoftParticle;
		spr.renderType = renderType;

		view.SpritesToDraw.push_back(spr);
	}

	void Renderer::AddColoredQuad(const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
		const Vector4& color, BlendMode blendMode, RenderView& view)
	{
		AddColoredQuad(vertex0, vertex1, vertex2, vertex3, color, color, color, color, blendMode, view, SpriteRenderType::Default);
	}

	void Renderer::AddColoredQuad(const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, const Vector3& vertex3,
		const Vector4& color0, const Vector4& color1, const Vector4& color2, const Vector4& color3,
		BlendMode blendMode, RenderView& view, SpriteRenderType renderType)
	{
		auto sprite = RendererSpriteToDraw{};

		sprite.Type = SpriteType::ThreeD;
		sprite.Sprite = &_whiteSprite;
		sprite.vtx1 = vertex0;
		sprite.vtx2 = vertex1;
		sprite.vtx3 = vertex2;
		sprite.vtx4 = vertex3;
		sprite.c1 = color0;
		sprite.c2 = color1;
		sprite.c3 = color2;
		sprite.c4 = color3;
		sprite.BlendMode = blendMode;
		sprite.pos = (vertex0 + vertex1 + vertex2 + vertex3) / 4.0f;
		sprite.SoftParticle = false;
		sprite.renderType = renderType;

		view.SpritesToDraw.push_back(sprite);
	}

	void Renderer::SortAndPrepareSprites(RenderView& view)
	{
		if (view.SpritesToDraw.empty())
		{
			return;
		}

		_spriteBuckets.clear();

		// Sort sprites by sprite and blend mode for faster batching.
		std::sort(
			view.SpritesToDraw.begin(),
			view.SpritesToDraw.end(),
			[](RendererSpriteToDraw& rDrawSprite0, RendererSpriteToDraw& rDrawSprite1)
			{
				if (rDrawSprite0.Sprite != rDrawSprite1.Sprite)
				{
					return (rDrawSprite0.Sprite > rDrawSprite1.Sprite);
				}
				else if (rDrawSprite0.BlendMode != rDrawSprite1.BlendMode)
				{
					return (rDrawSprite0.BlendMode > rDrawSprite1.BlendMode);
				}
				else
				{
					return (rDrawSprite0.Type > rDrawSprite1.Type);
				}
			}
		);

		// Group sprites to draw in buckets for instancing (billboards only).
		RendererSpriteBucket currentSpriteBucket;

		currentSpriteBucket.Sprite = view.SpritesToDraw[0].Sprite;
		currentSpriteBucket.BlendMode = view.SpritesToDraw[0].BlendMode;
		currentSpriteBucket.IsBillboard = view.SpritesToDraw[0].Type != SpriteType::ThreeD;
		currentSpriteBucket.IsSoftParticle = view.SpritesToDraw[0].SoftParticle;
		currentSpriteBucket.RenderType = view.SpritesToDraw[0].renderType;

		for (auto& rDrawSprite : view.SpritesToDraw)
		{
			bool isBillboard = rDrawSprite.Type != SpriteType::ThreeD;

			if (rDrawSprite.Sprite != currentSpriteBucket.Sprite ||
				rDrawSprite.BlendMode != currentSpriteBucket.BlendMode ||
				rDrawSprite.SoftParticle != currentSpriteBucket.IsSoftParticle ||
				rDrawSprite.renderType != currentSpriteBucket.RenderType ||
				currentSpriteBucket.SpritesToDraw.size() == INSTANCED_SPRITES_BUCKET_SIZE ||
				isBillboard != currentSpriteBucket.IsBillboard)
			{
				_spriteBuckets.push_back(currentSpriteBucket);

				currentSpriteBucket.Sprite = rDrawSprite.Sprite;
				currentSpriteBucket.BlendMode = rDrawSprite.BlendMode;
				currentSpriteBucket.IsBillboard = isBillboard;
				currentSpriteBucket.IsSoftParticle = rDrawSprite.SoftParticle;
				currentSpriteBucket.RenderType = rDrawSprite.renderType;
				currentSpriteBucket.SpritesToDraw.clear();
			}

			if (rDrawSprite.BlendMode != BlendMode::Opaque &&
				rDrawSprite.BlendMode != BlendMode::Additive &&
				rDrawSprite.BlendMode != BlendMode::AlphaTest)
			{
				int distance = (rDrawSprite.pos - Camera.pos.ToVector3()).Length();
				RendererSortableObject object;
				object.ObjectType = RendererObjectType::Sprite;
				object.Centre = rDrawSprite.pos;
				object.Distance = distance;
				object.Sprite = &rDrawSprite;

				view.TransparentObjectsToDraw.push_back(object);
			}
			else
			{
				currentSpriteBucket.SpritesToDraw.push_back(rDrawSprite);
			}
		}

		_spriteBuckets.push_back(currentSpriteBucket);
	}

	void Renderer::DrawSprites(RenderView& view, RendererPass rendererPass)
	{
		if (rendererPass == RendererPass::Additive)
		{
			static ID3D11Device* resourceDevice = nullptr;
			static WeatherGpuBuffer weatherGpuBuffer = {};
			static WeatherCpuBucket dustBucket = {};
			static std::vector<WeatherCpuBucket> snowBuckets = {};
			static std::vector<WeatherCpuBucket> rainBuckets = {};
			static std::vector<RendererWeatherParticle> weatherParticles = {};
			static WeatherTextureCache snowTextureCache = {};
			static WeatherTextureCache rainTextureCache = {};

			if (resourceDevice != _device.Get())
			{
				resourceDevice = _device.Get();
				weatherGpuBuffer = {};
				weatherParticles.clear();
				snowTextureCache = {};
				rainTextureCache = {};
			}

			const bool hasDefaultSprites = Objects[ID_DEFAULT_SPRITES].loaded;
			const bool hasDripSprite = Objects[ID_DRIP_SPRITE].loaded;
			const bool hasSnowSprites = Objects[ID_SNOW_SPRITES].loaded && Objects[ID_SNOW_SPRITES].nmeshes > 0;
			const bool hasRainSprites = Objects[ID_RAIN_SPRITES].loaded && Objects[ID_RAIN_SPRITES].nmeshes > 0;
			const int snowSpriteCount = hasSnowSprites ? Objects[ID_SNOW_SPRITES].nmeshes : (hasDefaultSprites ? 1 : 0);
			const int rainSpriteCount = hasRainSprites ? Objects[ID_RAIN_SPRITES].nmeshes : (hasDripSprite ? 1 : 0);

			dustBucket.Particles.clear();
			dustBucket.MaxClusterSize = 1;
			dustBucket.ParticleOffset = 0;
			dustBucket.Sprite = hasDefaultSprites ? &_sprites[Objects[ID_DEFAULT_SPRITES].meshIndex + SPR_UNDERWATERDUST] : nullptr;

			snowBuckets.resize(snowSpriteCount);
			for (int i = 0; i < snowSpriteCount; i++)
			{
				snowBuckets[i].Particles.clear();
				snowBuckets[i].MaxClusterSize = 1;
				snowBuckets[i].ParticleOffset = 0;
				const int spriteIndex = hasSnowSprites ? Objects[ID_SNOW_SPRITES].meshIndex + i : Objects[ID_DEFAULT_SPRITES].meshIndex + SPR_UNDERWATERDUST;
				snowBuckets[i].Sprite = &_sprites[spriteIndex];
			}

			rainBuckets.resize(rainSpriteCount);
			for (int i = 0; i < rainSpriteCount; i++)
			{
				rainBuckets[i].Particles.clear();
				rainBuckets[i].MaxClusterSize = 1;
				rainBuckets[i].ParticleOffset = 0;
				const int spriteIndex = hasRainSprites ? Objects[ID_RAIN_SPRITES].meshIndex + i : Objects[ID_DRIP_SPRITE].meshIndex;
				rainBuckets[i].Sprite = &_sprites[spriteIndex];
			}

			auto cacheMatches = [](const WeatherTextureCache& cache, const std::vector<WeatherCpuBucket>& buckets)
			{
				if (cache.Sources.size() != buckets.size())
					return false;

				for (size_t i = 0; i < buckets.size(); i++)
				{
					auto* sprite = buckets[i].Sprite;
					if (sprite == nullptr ||
						sprite->Texture == nullptr ||
						cache.Sources[i].Sprite != sprite ||
						cache.Sources[i].Texture != sprite->Texture ||
						cache.Sources[i].Resource != sprite->Texture->Texture.Get() ||
						memcmp(cache.Sources[i].UV, sprite->UV, sizeof(sprite->UV)) != 0)
					{
						return false;
					}
				}

				return true;
			};

			auto rebuildTextureCache = [&](WeatherTextureCache& cache, const std::vector<WeatherCpuBucket>& buckets)
			{
				cache = {};
				if (buckets.empty())
					return;

				auto frames = std::vector<RendererWeatherFrame>{};
				auto uniqueTextures = std::vector<Texture2D*>{};
				frames.reserve(buckets.size());
				cache.Sources.reserve(buckets.size());

				for (const auto& bucket : buckets)
				{
					auto* sprite = bucket.Sprite;
					if (sprite == nullptr || sprite->Texture == nullptr)
						return;

					auto textureIt = std::find(uniqueTextures.begin(), uniqueTextures.end(), sprite->Texture);
					if (textureIt == uniqueTextures.end())
					{
						uniqueTextures.push_back(sprite->Texture);
						textureIt = uniqueTextures.end() - 1;
					}

					auto frame = RendererWeatherFrame{};
					frame.UVX = Vector4(sprite->UV[0].x, sprite->UV[1].x, sprite->UV[2].x, sprite->UV[3].x);
					frame.UVY = Vector4(sprite->UV[0].y, sprite->UV[1].y, sprite->UV[2].y, sprite->UV[3].y);
					frame.TextureSlice = static_cast<int>(std::distance(uniqueTextures.begin(), textureIt));
					frames.push_back(frame);

					auto source = WeatherFrameSource{};
					source.Sprite = sprite;
					source.Texture = sprite->Texture;
					source.Resource = sprite->Texture->Texture.Get();
					memcpy(source.UV, sprite->UV, sizeof(sprite->UV));
					cache.Sources.push_back(source);
				}

				if (uniqueTextures.empty())
					return;

				if (uniqueTextures.size() == 1)
				{
					cache.AtlasTexture = uniqueTextures[0];
				}
				else
				{
					if (!Texture2DArray::AreCompatible(uniqueTextures))
						return;

					cache.TextureArray = Texture2DArray(_device.Get(), _context.Get(), uniqueTextures);
					cache.UsesTextureArray = true;
				}

				auto bufferDesc = D3D11_BUFFER_DESC{};
				bufferDesc.ByteWidth = static_cast<UINT>(sizeof(RendererWeatherFrame) * frames.size());
				bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
				bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				bufferDesc.CPUAccessFlags = 0;
				bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				bufferDesc.StructureByteStride = sizeof(RendererWeatherFrame);

				auto initialData = D3D11_SUBRESOURCE_DATA{};
				initialData.pSysMem = frames.data();
				Utils::throwIfFailed(_device->CreateBuffer(&bufferDesc, &initialData, cache.FrameBuffer.GetAddressOf()));

				auto viewDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
				viewDesc.Format = DXGI_FORMAT_UNKNOWN;
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				viewDesc.Buffer.FirstElement = 0;
				viewDesc.Buffer.NumElements = static_cast<UINT>(frames.size());
				Utils::throwIfFailed(_device->CreateShaderResourceView(cache.FrameBuffer.Get(), &viewDesc, cache.FrameView.GetAddressOf()));
				cache.CanBatch = true;
			};

			if (!cacheMatches(snowTextureCache, snowBuckets))
				rebuildTextureCache(snowTextureCache, snowBuckets);
			if (!cacheMatches(rainTextureCache, rainBuckets))
				rebuildTextureCache(rainTextureCache, rainBuckets);

			const auto interpolationFactor = GetInterpolationFactor();
			for (const auto& particle : Weather.GetGpuParticles())
			{
				if (!particle.Enabled)
					continue;

				auto position = Vector3::Lerp(particle.PrevPosition, particle.Position, interpolationFactor);
				auto velocity = particle.Velocity;
				const float size = Lerp(particle.PrevSize, particle.Size, interpolationFactor);
				const float cullRadius = particle.Type == WeatherType::None ? size : size + BLOCK(2.0f);

				if (!view.Camera.Frustum.SphereInFrustum(position, cullRadius))
					continue;

				if (_currentMirror != nullptr)
				{
					auto velocityEnd = position + velocity;
					ReflectVectorOptionally(position);
					ReflectVectorOptionally(velocityEnd);
					velocity = velocityEnd - position;
				}

				RendererWeatherParticle rendererParticle = {};
				rendererParticle.Position = position;
				rendererParticle.Size = size;
				rendererParticle.Velocity = velocity;
				rendererParticle.Opacity = particle.Transparency();
				rendererParticle.UniqueID = particle.UniqueID;
				rendererParticle.ClusterSize = particle.Stopped ? 1 : std::clamp(particle.ClusterSize, 1, GPU_WEATHER_CLUSTER_STRIDE);

				switch (particle.Type)
				{
				case WeatherType::None:
					if (dustBucket.Sprite != nullptr)
						dustBucket.Particles.push_back(rendererParticle);
					break;

				case WeatherType::Snow:
					if (!snowBuckets.empty())
					{
						const int frameIndex = particle.UniqueID % static_cast<int>(snowBuckets.size());
						rendererParticle.FrameIndex = frameIndex;
						auto& bucket = snowBuckets[frameIndex];
						bucket.MaxClusterSize = std::max(bucket.MaxClusterSize, rendererParticle.ClusterSize);
						bucket.Particles.push_back(rendererParticle);
					}
					break;

				case WeatherType::Rain:
					if (!rainBuckets.empty())
					{
						const int frameIndex = particle.UniqueID % static_cast<int>(rainBuckets.size());
						rendererParticle.FrameIndex = frameIndex;
						auto& bucket = rainBuckets[frameIndex];
						bucket.MaxClusterSize = std::max(bucket.MaxClusterSize, rendererParticle.ClusterSize);
						bucket.Particles.push_back(rendererParticle);
					}
					break;
				}
			}

			size_t weatherParticleCount = dustBucket.Particles.size();
			for (const auto& bucket : snowBuckets)
				weatherParticleCount += bucket.Particles.size();
			for (const auto& bucket : rainBuckets)
				weatherParticleCount += bucket.Particles.size();

			weatherParticles.clear();
			weatherParticles.reserve(weatherParticleCount);

			auto appendBucket = [&](WeatherCpuBucket& bucket)
			{
				bucket.ParticleOffset = static_cast<int>(weatherParticles.size());
				weatherParticles.insert(weatherParticles.end(), bucket.Particles.begin(), bucket.Particles.end());
			};

			appendBucket(dustBucket);
			for (auto& bucket : snowBuckets)
				appendBucket(bucket);
			for (auto& bucket : rainBuckets)
				appendBucket(bucket);

			auto uploadBuffer = [&](WeatherGpuBuffer& gpuBuffer, const std::vector<RendererWeatherParticle>& particles)
			{
				if (particles.empty())
					return false;

				if (gpuBuffer.Capacity < particles.size())
				{
					size_t capacity = 1;
					while (capacity < particles.size())
						capacity <<= 1;

					auto bufferDesc = D3D11_BUFFER_DESC{};
					bufferDesc.ByteWidth = static_cast<UINT>(sizeof(RendererWeatherParticle) * capacity);
					bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
					bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
					bufferDesc.StructureByteStride = sizeof(RendererWeatherParticle);

					gpuBuffer.Buffer.Reset();
					gpuBuffer.View.Reset();
					Utils::throwIfFailed(_device->CreateBuffer(&bufferDesc, nullptr, gpuBuffer.Buffer.GetAddressOf()));

					auto viewDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
					viewDesc.Format = DXGI_FORMAT_UNKNOWN;
					viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
					viewDesc.Buffer.FirstElement = 0;
					viewDesc.Buffer.NumElements = static_cast<UINT>(capacity);
					Utils::throwIfFailed(_device->CreateShaderResourceView(gpuBuffer.Buffer.Get(), &viewDesc, gpuBuffer.View.GetAddressOf()));
					gpuBuffer.Capacity = capacity;
				}

				auto mappedResource = D3D11_MAPPED_SUBRESOURCE{};
				Utils::throwIfFailed(_context->Map(gpuBuffer.Buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
				memcpy(mappedResource.pData, particles.data(), particles.size() * sizeof(RendererWeatherParticle));
				_context->Unmap(gpuBuffer.Buffer.Get(), 0);
				return true;
			};

			auto packEnvironmentTextureCoordinates = [&](RendererSprite* sprite)
			{
				_stStarfield.UV[0].x = sprite->UV[0].x;
				_stStarfield.UV[0].y = sprite->UV[1].x;
				_stStarfield.UV[0].z = sprite->UV[2].x;
				_stStarfield.UV[0].w = sprite->UV[3].x;
				_stStarfield.UV[1].x = sprite->UV[0].y;
				_stStarfield.UV[1].y = sprite->UV[1].y;
				_stStarfield.UV[1].z = sprite->UV[2].y;
				_stStarfield.UV[1].w = sprite->UV[3].y;
			};

			if (!weatherParticles.empty())
			{
				_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				_context->IASetInputLayout(_inputLayout.Get());
				unsigned int stride = sizeof(Vertex);
				unsigned int offset = 0;
				_context->IASetVertexBuffers(0, 1, _quadVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);

				BindRenderTargetAsTexture(TextureRegister::GBufferDepthMap, &_depthRenderTarget, SamplerStateRegister::PointWrap);
				SetDepthState(DepthState::Read);
				SetBlendMode(BlendMode::Additive);
				SetCullMode(CullMode::None);
				SetAlphaTest(AlphaTestMode::None, ALPHA_TEST_THRESHOLD);
				_shaders.Bind(Shader::Starfield);

				if (uploadBuffer(weatherGpuBuffer, weatherParticles))
				{
					auto* weatherView = weatherGpuBuffer.View.Get();
					_context->VSSetShaderResources(WEATHER_BUFFER_SLOT, 1, &weatherView);
					BindConstantBufferVS(ConstantBufferRegister::InstancedSprites, _cbStarfield.get());
					BindConstantBufferPS(ConstantBufferRegister::InstancedSprites, _cbStarfield.get());

					auto drawBucket = [&](WeatherCpuBucket& bucket, GpuEnvironmentMode mode)
					{
						if (bucket.Particles.empty())
							return;

						const int clusterStride = std::clamp(bucket.MaxClusterSize, 1, GPU_WEATHER_CLUSTER_STRIDE);
						packEnvironmentTextureCoordinates(bucket.Sprite);
						_stStarfield.Mode = mode;
						_stStarfield.ClusterStride = clusterStride;
						_stStarfield.ClusterSpread = mode == GpuEnvironmentMode::UnderwaterDust ? 0.0f : BLOCK(1.0f);
						_stStarfield.ParticleOffset = bucket.ParticleOffset;
						_stStarfield.TextureMode = GpuEnvironmentTextureMode::Bucket;
						UpdateConstantBuffer(_stStarfield, _cbStarfield);
						BindTexture(TextureRegister::ColorMap, bucket.Sprite->Texture, SamplerStateRegister::LinearClamp);

						DrawInstancedTriangles(4, static_cast<int>(bucket.Particles.size()) * clusterStride, 0);
						_numInstancedSpritesDrawCalls++;
					};

					auto drawWeatherType = [&](std::vector<WeatherCpuBucket>& buckets, WeatherTextureCache& cache, GpuEnvironmentMode mode)
					{
						int particleCount = 0;
						int maxClusterSize = 1;
						int particleOffset = 0;
						bool foundParticles = false;

						for (const auto& bucket : buckets)
						{
							if (bucket.Particles.empty())
								continue;

							if (!foundParticles)
							{
								particleOffset = bucket.ParticleOffset;
								foundParticles = true;
							}

							particleCount += static_cast<int>(bucket.Particles.size());
							maxClusterSize = std::max(maxClusterSize, bucket.MaxClusterSize);
						}

						if (!foundParticles)
							return;

						if (!cache.CanBatch)
						{
							for (auto& bucket : buckets)
								drawBucket(bucket, mode);
							return;
						}

						auto* frameView = cache.FrameView.Get();
						_context->VSSetShaderResources(WEATHER_FRAME_BUFFER_SLOT, 1, &frameView);

						_stStarfield.Mode = mode;
						_stStarfield.ClusterStride = std::clamp(maxClusterSize, 1, GPU_WEATHER_CLUSTER_STRIDE);
						_stStarfield.ClusterSpread = BLOCK(1.0f);
						_stStarfield.ParticleOffset = particleOffset;

						if (cache.UsesTextureArray)
						{
							_stStarfield.TextureMode = GpuEnvironmentTextureMode::Array;
							auto* textureArrayView = cache.TextureArray.ShaderResourceView.Get();
							_context->PSSetShaderResources(WEATHER_TEXTURE_ARRAY_SLOT, 1, &textureArrayView);
						}
						else
						{
							_stStarfield.TextureMode = GpuEnvironmentTextureMode::Atlas;
							BindTexture(TextureRegister::ColorMap, cache.AtlasTexture, SamplerStateRegister::LinearClamp);
						}

						UpdateConstantBuffer(_stStarfield, _cbStarfield);
						DrawInstancedTriangles(4, particleCount * _stStarfield.ClusterStride, 0);
						_numInstancedSpritesDrawCalls++;
					};

					drawBucket(dustBucket, GpuEnvironmentMode::UnderwaterDust);
					drawWeatherType(snowBuckets, snowTextureCache, GpuEnvironmentMode::Snow);
					drawWeatherType(rainBuckets, rainTextureCache, GpuEnvironmentMode::Rain);
				}

				ID3D11ShaderResourceView* nullView = nullptr;
				_context->VSSetShaderResources(WEATHER_BUFFER_SLOT, 1, &nullView);
				_context->VSSetShaderResources(WEATHER_FRAME_BUFFER_SLOT, 1, &nullView);
				_context->PSSetShaderResources(WEATHER_TEXTURE_ARRAY_SLOT, 1, &nullView);
				BindConstantBufferVS(ConstantBufferRegister::InstancedSprites, _cbInstancedSpriteBuffer.get());
				BindConstantBufferPS(ConstantBufferRegister::InstancedSprites, _cbInstancedSpriteBuffer.get());
				_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}
		}

		if (view.SpritesToDraw.empty())
			return;

		// Draw instanced sprites.
		bool wasGpuSet = false;
		for (const auto& spriteBucket : _spriteBuckets)
		{
			if (spriteBucket.SpritesToDraw.empty() || !spriteBucket.IsBillboard)
				continue;

			if (!SetupBlendModeAndAlphaTest(spriteBucket.BlendMode, rendererPass, 0))
				continue;

			if (!wasGpuSet)
			{
				_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

				BindRenderTargetAsTexture(TextureRegister::GBufferDepthMap, &_depthRenderTarget, SamplerStateRegister::PointWrap);

				SetDepthState(DepthState::Read);
				SetCullMode(CullMode::None);

				_shaders.Bind(Shader::InstancedSprites);

				// Set up vertex buffer and parameters.
				unsigned int stride = sizeof(Vertex);
				unsigned int offset = 0;
				_context->IASetVertexBuffers(0, 1, _quadVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);

				wasGpuSet = true;
			}

			// Define sprite preparation logic.
			auto prepareSprites = [&](int start, int end)
			{
				for (int i = start; i < end; i++)
				{
					const auto& spriteToDraw = spriteBucket.SpritesToDraw[i];

					_stInstancedSpriteBuffer.Sprites[i].World = GetWorldMatrixForSprite(spriteToDraw, view);
					_stInstancedSpriteBuffer.Sprites[i].Color = spriteToDraw.color;
					_stInstancedSpriteBuffer.Sprites[i].IsBillboard = 1.0f;
					_stInstancedSpriteBuffer.Sprites[i].PerVertexColor = 0;
					_stInstancedSpriteBuffer.Sprites[i].IsSoftParticle = spriteToDraw.SoftParticle ? 1.0f : 0.0f;
					_stInstancedSpriteBuffer.Sprites[i].RenderType = (int)spriteToDraw.renderType;

					PackSpriteTextureCoordinates(i, spriteToDraw.Sprite);
				}
			};
			g_Parallel.AddTasks((int)spriteBucket.SpritesToDraw.size(), prepareSprites).wait();

			BindTexture(TextureRegister::ColorMap, spriteBucket.Sprite->Texture, SamplerStateRegister::LinearClamp);
			UpdateConstantBuffer(_stInstancedSpriteBuffer, _cbInstancedSpriteBuffer);;

			// Draw sprites with instancing.
			DrawInstancedTriangles(4, (int)spriteBucket.SpritesToDraw.size(), 0);

			_numInstancedSpritesDrawCalls++;
		}

		// Draw 3D non-instanced sprites.
		wasGpuSet = false;
		for (const auto& spriteBucket : _spriteBuckets)
		{
			if (spriteBucket.SpritesToDraw.empty() || spriteBucket.IsBillboard)
				continue;

			if (!SetupBlendModeAndAlphaTest(spriteBucket.BlendMode, rendererPass, 0))
				continue;

			if (!wasGpuSet)
			{
				_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				BindRenderTargetAsTexture(TextureRegister::GBufferDepthMap, &_depthRenderTarget, SamplerStateRegister::PointWrap);

				SetDepthState(DepthState::Read);
				SetCullMode(CullMode::None);

				_shaders.Bind(Shader::InstancedSprites);

				// Set up vertex buffer and parameters.
				unsigned int stride = sizeof(Vertex);
				unsigned int offset = 0;
				_context->IASetVertexBuffers(0, 1, _spriteVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);

				wasGpuSet = true;
			}
			
			_stInstancedSpriteBuffer.Sprites[0].IsBillboard = 0;
			_stInstancedSpriteBuffer.Sprites[0].World = Matrix::Identity;
			_stInstancedSpriteBuffer.Sprites[0].IsSoftParticle = spriteBucket.IsSoftParticle ? 1.0f : 0.0f;
			_stInstancedSpriteBuffer.Sprites[0].RenderType = (int)spriteBucket.RenderType;

			_stInstancedSpriteBuffer.Sprites[0].PerVertexColor = 1;
			_stInstancedSpriteBuffer.Sprites[0].IsSoftParticle = spriteBucket.IsSoftParticle ? 1.0f : 0.0f;

			PackSpriteTextureCoordinates(0, spriteBucket.Sprite);

			UpdateConstantBuffer(_stInstancedSpriteBuffer, _cbInstancedSpriteBuffer);

			BindTexture(TextureRegister::ColorMap, spriteBucket.Sprite->Texture, SamplerStateRegister::LinearClamp);

			int spritesToDraw = 0;

			for (auto& rDrawSprite : spriteBucket.SpritesToDraw)
			{
				auto vertex0 = Vertex{};
				vertex0.Position = rDrawSprite.vtx1;
				vertex0.UV = rDrawSprite.Sprite->UV[0];
				vertex0.Color = VectorColorToRGBA_TempToVector4(rDrawSprite.c1);
				vertex0.Effects = 0 << INDEX_IN_POLY_VERTEX_SHIFT;

				ReflectVectorOptionally(vertex0.Position);

				auto vertex1 = Vertex{};
				vertex1.Position = rDrawSprite.vtx2;
				vertex1.UV = rDrawSprite.Sprite->UV[1];
				vertex1.Color = VectorColorToRGBA_TempToVector4(rDrawSprite.c2);
				vertex1.Effects = 1 << INDEX_IN_POLY_VERTEX_SHIFT;

				ReflectVectorOptionally(vertex1.Position);

				auto vertex2 = Vertex{};
				vertex2.Position = rDrawSprite.vtx3;
				vertex2.UV = rDrawSprite.Sprite->UV[2];
				vertex2.Color = VectorColorToRGBA_TempToVector4(rDrawSprite.c3);
				vertex2.Effects = 2 << INDEX_IN_POLY_VERTEX_SHIFT;

				ReflectVectorOptionally(vertex2.Position);

				auto vertex3 = Vertex{};
				vertex3.Position = rDrawSprite.vtx4;
				vertex3.UV = rDrawSprite.Sprite->UV[3];
				vertex3.Color = VectorColorToRGBA_TempToVector4(rDrawSprite.c4);
				vertex3.Effects = 3 << INDEX_IN_POLY_VERTEX_SHIFT;

				ReflectVectorOptionally(vertex3.Position);

				_spriteVertices.push_back(vertex0);
				_spriteVertices.push_back(vertex1);
				_spriteVertices.push_back(vertex3);
				_spriteVertices.push_back(vertex2);
				_spriteVertices.push_back(vertex3);
				_spriteVertices.push_back(vertex1);

				spritesToDraw++;

				if (spritesToDraw == INSTANCED_SPRITES_BUCKET_SIZE || spritesToDraw == spriteBucket.SpritesToDraw.size())
				{
					_spriteVertexBuffer.Update(_context.Get(), _spriteVertices.data(), 0, spritesToDraw * 6);

					DrawInstancedTriangles(spritesToDraw * 6, 1, 0);

					_numInstancedSpritesDrawCalls++;

					spritesToDraw = 0;
					_spriteVertices.clear();
				}
		}
		}

		// Set up vertex parameters.
		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void Renderer::DrawSingleSprite(RendererSortableObject* object, RendererObjectType lastObjectType, RenderView& view)
	{
		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		BindRenderTargetAsTexture(TextureRegister::GBufferDepthMap, &_depthRenderTarget, SamplerStateRegister::LinearClamp);

		SetDepthState(DepthState::Read);
		SetCullMode(CullMode::None);
		SetBlendMode(object->Sprite->BlendMode);
		SetAlphaTest(AlphaTestMode::GreatherThan, ALPHA_TEST_THRESHOLD);

		_shaders.Bind(Shader::InstancedSprites);

		_stInstancedSpriteBuffer.Sprites[0].World = object->Sprite->Type != SpriteType::ThreeD ?
			GetWorldMatrixForSprite(*object->Sprite, view) :
			Matrix::Identity;
		_stInstancedSpriteBuffer.Sprites[0].PerVertexColor = 1;
		_stInstancedSpriteBuffer.Sprites[0].IsSoftParticle = object->Sprite->SoftParticle ? 1 : 0;
		_stInstancedSpriteBuffer.Sprites[0].RenderType = (int)object->Sprite->renderType;

		PackSpriteTextureCoordinates(0, object->Sprite->Sprite);

		UpdateConstantBuffer(_stInstancedSpriteBuffer, _cbInstancedSpriteBuffer);;

		BindTexture(TextureRegister::ColorMap, object->Sprite->Sprite->Texture, SamplerStateRegister::LinearClamp);
		
		// Set up vertex buffer and parameters.
		unsigned int stride = sizeof(Vertex);
		unsigned int offset = 0;

		if (object->Sprite->Type != SpriteType::ThreeD)
		{
			_context->IASetVertexBuffers(0, 1, _quadVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);
		}
		else
		{
			auto vertex0 = Vertex{};
			vertex0.Position = object->Sprite->vtx1;
			vertex0.UV = object->Sprite->Sprite->UV[0];
			vertex0.Color = VectorColorToRGBA_TempToVector4(object->Sprite->c1);
			vertex0.Effects = 0 << INDEX_IN_POLY_VERTEX_SHIFT;

			auto vertex1 = Vertex{};
			vertex1.Position = object->Sprite->vtx2;
			vertex1.UV = object->Sprite->Sprite->UV[1];
			vertex1.Color = VectorColorToRGBA_TempToVector4(object->Sprite->c2);
			vertex1.Effects = 1 << INDEX_IN_POLY_VERTEX_SHIFT;

			auto vertex2 = Vertex{};
			vertex2.Position = object->Sprite->vtx3;
			vertex2.UV = object->Sprite->Sprite->UV[2];
			vertex2.Color = VectorColorToRGBA_TempToVector4(object->Sprite->c3);
			vertex2.Effects = 2 << INDEX_IN_POLY_VERTEX_SHIFT;

			auto vertex3 = Vertex{};
			vertex3.Position = object->Sprite->vtx4;
			vertex3.UV = object->Sprite->Sprite->UV[3];
			vertex3.Color = VectorColorToRGBA_TempToVector4(object->Sprite->c4);
			vertex3.Effects = 3 << INDEX_IN_POLY_VERTEX_SHIFT;

			_spriteVertices.clear();
			_spriteVertices.push_back(vertex0);
			_spriteVertices.push_back(vertex1);
			_spriteVertices.push_back(vertex3);
			_spriteVertices.push_back(vertex2);

			_spriteVertexBuffer.Update(_context.Get(), _spriteVertices.data(), 0, 4);

			_context->IASetVertexBuffers(0, 1, _spriteVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);
		}

		// Draw sprites with instancing.
		DrawInstancedTriangles(4, 1, 0);

		_numSortedSpritesDrawCalls++;
		_numSortedTriangles += 2;

		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void Renderer::DrawSpriteSorted(RendererSortableObject* objectInfo, RendererObjectType lastObjectType, RenderView& view)
	{
		if (lastObjectType != objectInfo->ObjectType)
		{
			unsigned int stride = sizeof(Vertex);
			unsigned int offset = 0;

			_shaders.Bind(Shader::InstancedSprites);

			_context->IASetVertexBuffers(0, 1, _sortedPolygonsVertexBuffer.Buffer.GetAddressOf(), &stride, &offset);
			_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			_context->IASetInputLayout(_inputLayout.Get());
		}

		_sortedPolygonsVertexBuffer.Update(_context.Get(), _sortedPolygonsVertices.data(), 0, (int)_sortedPolygonsVertices.size());

		_stInstancedSpriteBuffer.Sprites[0].World = Matrix::Identity;
		_stInstancedSpriteBuffer.Sprites[0].PerVertexColor = 1;
		_stInstancedSpriteBuffer.Sprites[0].IsSoftParticle = objectInfo->Sprite->SoftParticle ? 1 : 0;
		_stInstancedSpriteBuffer.Sprites[0].RenderType = (int)objectInfo->Sprite->renderType;

		PackSpriteTextureCoordinates(0, objectInfo->Sprite->Sprite);

		UpdateConstantBuffer(_stInstancedSpriteBuffer, _cbInstancedSpriteBuffer);;

		SetDepthState(DepthState::Read);
		SetCullMode(CullMode::None);
		SetBlendMode(objectInfo->Sprite->BlendMode);
		SetAlphaTest(AlphaTestMode::None, ALPHA_TEST_THRESHOLD);

		BindTexture(TextureRegister::ColorMap, objectInfo->Sprite->Sprite->Texture, SamplerStateRegister::LinearClamp);
		BindRenderTargetAsTexture(TextureRegister::GBufferDepthMap, &_depthRenderTarget, SamplerStateRegister::PointWrap);

		DrawInstancedTriangles((int)_sortedPolygonsVertices.size(), 1, 0);

		_numSortedSpritesDrawCalls++;
		_numSortedTriangles += (int)_sortedPolygonsVertices.size() / 3;
	}

	void Renderer::PackSpriteTextureCoordinates(int instanceId, RendererSprite* sprite)
	{
		// NOTE: Strange packing due to particular HLSL 16 byte alignment requirements.
	
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[0].x = sprite->UV[0].x;
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[0].y = sprite->UV[1].x;
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[0].z = sprite->UV[2].x;
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[0].w = sprite->UV[3].x;
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[1].x = sprite->UV[0].y;
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[1].y = sprite->UV[1].y;
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[1].z = sprite->UV[2].y;
		_stInstancedSpriteBuffer.Sprites[instanceId].UV[1].w = sprite->UV[3].y;
	}
}
