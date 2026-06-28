#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Math/Math.h"
#include "Specific/level.h"

namespace TEN::Effects::HDRLight
{
	using namespace DirectX::SimpleMath;

	constexpr uint8_t ROOM_LIGHT_TYPE = static_cast<uint8_t>(LightType::HDR);
	constexpr uint8_t TRANSPORT_LIGHT_TYPE = static_cast<uint8_t>(LightType::Spot);
	constexpr float TRANSPORT_MARKER = -0.001f;
	constexpr float TRANSPORT_CORE_SCALE = 0.001f;
	constexpr float TRANSPORT_SOURCE_HEIGHT_SCALE = 0.00000001f;
	constexpr float TRANSPORT_MODE_SECTOR_DEGREES = 120.0f;
	constexpr float TRANSPORT_INTENSITY_DEGREES_PER_UNIT = 10.0f;
	constexpr float TRANSPORT_GLARE_DEGREES_PER_UNIT = 10.0f;
	constexpr int MAX_EFFECT_LAYERS = 16;
	constexpr int MAX_VISIBLE_EFFECT_LAYERS = 512;

	enum class Mode : uint8_t
	{
		LightAndEffects,
		LightOnly,
		EffectsOnly
	};

	enum class PhysicalType : uint8_t
	{
		Point,
		Spot
	};

	enum class EffectType : uint8_t
	{
		SourceCore,
		Halo,
		Glare
	};

	struct EffectLayer
	{
		EffectType Type = EffectType::SourceCore;
		Vector3 ColorMultiplier = Vector3::One;
		Vector2 Size = Vector2(64.0f, 64.0f);
		float Intensity = 1.0f;
		float Softness = 0.5f;
		float Rotation = 0.0f;
		float PulseAmount = 0.0f;
		float PulseSpeed = 1.0f;
		float MaxDistance = BLOCK(32);
		int RayCount = 4;
		bool Occlusion = true;
		bool Enabled = true;
	};

	struct Definition
	{
		Vector3 Position = Vector3::Zero;
		Vector3 Direction = Vector3::UnitZ;
		Vector3 Color = Vector3::One;
		int RoomNumber = NO_VALUE;
		int Hash = 0;

		Mode LightMode = Mode::LightAndEffects;
		PhysicalType LightType = PhysicalType::Point;
		float PhysicalIntensity = 1.0f;
		float InnerRange = 0.0f;
		float OuterRange = BLOCK(4);
		float SpotRadius = CLICK(2);
		float SpotFalloff = CLICK(1);
		bool CastShadows = false;
		bool AffectNeighbourRooms = true;

		std::vector<EffectLayer> Effects = {};
	};

	inline std::vector<Definition> LevelLights = {};
	inline std::vector<Definition> RuntimeLights = {};

	inline EffectLayer MakeSourceCore(const Vector2& size, float intensity)
	{
		auto effect = EffectLayer{};
		effect.Type = EffectType::SourceCore;
		effect.Size = size;
		effect.Intensity = intensity;
		effect.Softness = 0.72f;
		return effect;
	}

	inline EffectLayer MakeHalo(const Vector2& size, float intensity)
	{
		auto effect = EffectLayer{};
		effect.Type = EffectType::Halo;
		effect.Size = size;
		effect.Intensity = intensity;
		effect.Softness = 0.38f;
		return effect;
	}

	inline EffectLayer MakeGlare(const Vector2& size, float intensity)
	{
		auto effect = EffectLayer{};
		effect.Type = EffectType::Glare;
		effect.Size = size;
		effect.Intensity = intensity;
		effect.Softness = 0.5f;
		effect.RayCount = 4;
		return effect;
	}

	inline bool IsTransportRecord(const RoomLightData& source)
	{
		return source.type == TRANSPORT_LIGHT_TYPE && source.in <= TRANSPORT_MARKER;
	}

	inline Definition ConvertTransportLight(const RoomLightData& source, int roomNumber, int lightIndex)
	{
		auto light = Definition{};
		light.Position = Vector3((float)source.x, (float)source.y, (float)source.z);
		light.Color = Vector3(source.r, source.g, source.b);
		light.RoomNumber = roomNumber;
		light.Hash = 0x48440000 ^ ((roomNumber & 0x7FF) << 12) ^ (lightIndex & 0xFFF);
		light.LightType = PhysicalType::Point;
		light.InnerRange = 0.0f;
		light.OuterRange = std::max(std::abs(source.length), 1.0f);
		light.CastShadows = source.castShadows;

		const float clampedDirectionY = std::clamp(source.dy, -1.0f, 1.0f);
		const float encodedGlareDegrees = std::abs(std::asin(clampedDirectionY) * (180.0f / PI));
		const float glareIntensity = encodedGlareDegrees / TRANSPORT_GLARE_DEGREES_PER_UNIT;

		float encodedYaw = std::atan2(source.dx, source.dz) * (180.0f / PI);
		if (encodedYaw < 0.0f)
			encodedYaw += 360.0f;

		const int modeIndex = std::clamp(
			(int)std::floor(encodedYaw / TRANSPORT_MODE_SECTOR_DEGREES),
			0,
			2);
		light.LightMode = (Mode)modeIndex;

		const float intensityAngle = encodedYaw - modeIndex * TRANSPORT_MODE_SECTOR_DEGREES;
		light.PhysicalIntensity = std::clamp(
			intensityAngle / TRANSPORT_INTENSITY_DEGREES_PER_UNIT,
			0.0f,
			10.0f);

		const float sourceWidth = std::max(std::abs(source.cutoff), 32.0f);
		const float sourceHeightSectors = std::max(
			source.intensity / TRANSPORT_SOURCE_HEIGHT_SCALE - 1.0f,
			0.01f);
		const float sourceHeight = std::max(sourceHeightSectors * BLOCK(1), 32.0f);
		const Vector2 sourceSize(sourceWidth, sourceHeight);
		const float coreIntensity = std::max(
			(TRANSPORT_MARKER - source.in) / TRANSPORT_CORE_SCALE,
			0.0f);
		const float haloIntensity = std::max(source.out, 0.0f);

		if (light.LightMode != Mode::LightOnly)
		{
			light.Effects.reserve(3);
			if (coreIntensity > EPSILON)
				light.Effects.push_back(MakeSourceCore(sourceSize, coreIntensity));
			if (haloIntensity > EPSILON)
				light.Effects.push_back(MakeHalo(sourceSize * 4.0f, haloIntensity));
			if (glareIntensity > EPSILON)
			{
				light.Effects.push_back(MakeGlare(
					Vector2(sourceWidth * 8.0f, std::max(sourceHeight * 1.5f, 32.0f)),
					glareIntensity));
			}
		}

		return light;
	}

	inline Definition ConvertNativeRoomLight(const RoomLightData& source, int roomNumber, int lightIndex)
	{
		auto light = Definition{};
		light.Position = Vector3((float)source.x, (float)source.y, (float)source.z);
		light.Direction = Vector3(source.dx, source.dy, source.dz);
		if (light.Direction.LengthSquared() <= EPSILON)
			light.Direction = Vector3::UnitZ;
		else
			light.Direction.Normalize();

		light.Color = Vector3(source.r, source.g, source.b);
		light.RoomNumber = roomNumber;
		light.Hash = 0x48440000 ^ ((roomNumber & 0x7FF) << 12) ^ (lightIndex & 0xFFF);
		light.PhysicalIntensity = std::max(source.intensity, 0.0f);
		light.InnerRange = std::max(source.in, 0.0f);
		light.OuterRange = std::max(source.out, light.InnerRange + 1.0f);
		light.CastShadows = source.castShadows;

		const float sourceWidth = std::max(std::abs(source.length), 32.0f);
		const float sourceHeight = std::max(std::abs(source.cutoff), sourceWidth);
		const Vector2 sourceSize(sourceWidth, sourceHeight);

		light.Effects.reserve(3);
		light.Effects.push_back(MakeSourceCore(sourceSize, std::max(2.0f, source.intensity * 4.0f)));
		light.Effects.push_back(MakeHalo(sourceSize * 4.0f, std::max(0.65f, source.intensity * 1.4f)));
		light.Effects.push_back(MakeGlare(
			Vector2(sourceWidth * 8.0f, std::max(sourceHeight * 1.5f, 32.0f)),
			std::max(0.35f, source.intensity * 0.8f)));

		return light;
	}

	inline void RefreshLevelLights()
	{
		LevelLights.clear();

		for (int roomNumber = 0; roomNumber < (int)g_Level.Rooms.size(); roomNumber++)
		{
			const auto& room = g_Level.Rooms[roomNumber];
			for (int lightIndex = 0; lightIndex < (int)room.lights.size(); lightIndex++)
			{
				const auto& source = room.lights[lightIndex];
				if (IsTransportRecord(source))
				{
					LevelLights.push_back(ConvertTransportLight(source, roomNumber, lightIndex));
					continue;
				}

				if (source.type == ROOM_LIGHT_TYPE)
					LevelLights.push_back(ConvertNativeRoomLight(source, roomNumber, lightIndex));
			}
		}
	}

	inline void AddRuntimeLight(const Definition& light)
	{
		RuntimeLights.push_back(light);
	}

	inline void ClearRuntimeLights()
	{
		RuntimeLights.clear();
	}

	template <typename Callback>
	inline void ForEachLight(Callback&& callback)
	{
		for (const auto& light : LevelLights)
			callback(light);
		for (const auto& light : RuntimeLights)
			callback(light);
	}

	template <typename RendererType>
	inline void SubmitPhysicalLights(RendererType& renderer)
	{
		ForEachLight([&](const Definition& light)
		{
			if (light.LightMode == Mode::EffectsOnly || light.PhysicalIntensity <= EPSILON)
				return;

			const auto color = Color(
				light.Color.x * light.PhysicalIntensity,
				light.Color.y * light.PhysicalIntensity,
				light.Color.z * light.PhysicalIntensity,
				1.0f);

			if (light.LightType == PhysicalType::Spot)
			{
				renderer.AddDynamicSpotLight(
					light.Position,
					light.Direction,
					std::max(light.SpotRadius, 1.0f),
					std::max(light.SpotFalloff, 0.0f),
					std::max(light.OuterRange, 1.0f),
					color,
					light.CastShadows,
					light.Hash);
			}
			else
			{
				renderer.AddDynamicPointLight(
					light.Position,
					std::max(light.OuterRange, 1.0f),
					color,
					light.CastShadows,
					light.Hash);
			}
		});
	}
}
