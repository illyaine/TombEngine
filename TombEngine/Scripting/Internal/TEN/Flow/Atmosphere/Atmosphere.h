#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Scripting/Include/ScriptInterfaceLevel.h"
#include "Scripting/Internal/TEN/Types/Color/Color.h"

namespace TEN::Scripting
{
	enum class WeatherQuality
	{
		Low,
		Medium,
		High,
		Ultra,
		Auto
	};

	enum class AtmosphereEffectType
	{
		None,
		GroundFog,
		Mist,
		SnowstormVeil,
		SandstormVeil,
		DustSheet,
		AshFall,
		LeafFall,
		MagicParticles,
		Custom
	};

	enum class AtmosphereEffectScope
	{
		Global,
		Nullmesh,
		Room,
		Volume
	};

	enum class AtmosphereEffectRenderMode
	{
		Generated,
		Sprite,
		ExistingEffect,
		Custom
	};

	/// Constants for weather quality budgets.
	// @enum Flow.WeatherQuality
	// @pragma nostrip
	static const std::unordered_map<std::string, WeatherQuality> WEATHER_QUALITIES
	{
		/// Low weather quality budget.
		// @mem Low
		{ "Low", WeatherQuality::Low },

		/// Medium weather quality budget.
		// @mem Medium
		{ "Medium", WeatherQuality::Medium },

		/// High weather quality budget.
		// @mem High
		{ "High", WeatherQuality::High },

		/// Ultra weather quality budget.
		// @mem Ultra
		{ "Ultra", WeatherQuality::Ultra },

		/// Automatic weather quality budget.
		// @mem Auto
		{ "Auto", WeatherQuality::Auto }
	};

	/// Constants for generated or bridged atmosphere effect types.
	// @enum Flow.AtmosphereEffectType
	// @pragma nostrip
	static const std::unordered_map<std::string, AtmosphereEffectType> ATMOSPHERE_EFFECT_TYPES
	{
		/// No atmosphere effect.
		// @mem None
		{ "None", AtmosphereEffectType::None },

		/// Generated ground fog.
		// @mem GroundFog
		{ "GroundFog", AtmosphereEffectType::GroundFog },

		/// Generated drifting mist.
		// @mem Mist
		{ "Mist", AtmosphereEffectType::Mist },

		/// Generated snowstorm veil.
		// @mem SnowstormVeil
		{ "SnowstormVeil", AtmosphereEffectType::SnowstormVeil },

		/// Generated sandstorm veil.
		// @mem SandstormVeil
		{ "SandstormVeil", AtmosphereEffectType::SandstormVeil },

		/// Generated dust sheet.
		// @mem DustSheet
		{ "DustSheet", AtmosphereEffectType::DustSheet },

		/// Generated ash fall.
		// @mem AshFall
		{ "AshFall", AtmosphereEffectType::AshFall },

		/// Generated leaf fall.
		// @mem LeafFall
		{ "LeafFall", AtmosphereEffectType::LeafFall },

		/// Generated magic particles.
		// @mem MagicParticles
		{ "MagicParticles", AtmosphereEffectType::MagicParticles },

		/// Custom atmosphere effect.
		// @mem Custom
		{ "Custom", AtmosphereEffectType::Custom }
	};

	/// Constants for atmosphere effect scopes.
	// @enum Flow.AtmosphereEffectScope
	// @pragma nostrip
	static const std::unordered_map<std::string, AtmosphereEffectScope> ATMOSPHERE_EFFECT_SCOPES
	{
		/// Global atmosphere effect.
		// @mem Global
		{ "Global", AtmosphereEffectScope::Global },

		/// Nullmesh or object anchored atmosphere effect.
		// @mem Nullmesh
		{ "Nullmesh", AtmosphereEffectScope::Nullmesh },

		/// Room-bound atmosphere effect.
		// @mem Room
		{ "Room", AtmosphereEffectScope::Room },

		/// Volume-bound atmosphere effect.
		// @mem Volume
		{ "Volume", AtmosphereEffectScope::Volume }
	};

	/// Constants for atmosphere effect render modes.
	// @enum Flow.AtmosphereEffectRenderMode
	// @pragma nostrip
	static const std::unordered_map<std::string, AtmosphereEffectRenderMode> ATMOSPHERE_EFFECT_RENDER_MODES
	{
		/// Engine-generated visual layer.
		// @mem Generated
		{ "Generated", AtmosphereEffectRenderMode::Generated },

		/// Optional external sprite or texture path.
		// @mem Sprite
		{ "Sprite", AtmosphereEffectRenderMode::Sprite },

		/// Bridge to an existing engine effect.
		// @mem ExistingEffect
		{ "ExistingEffect", AtmosphereEffectRenderMode::ExistingEffect },

		/// Custom or future renderer-specific effect path.
		// @mem Custom
		{ "Custom", AtmosphereEffectRenderMode::Custom }
	};

	struct RainProfile
	{
		float WindInfluence{ 0.0f };
		float NearDensity{ 1.0f };
		float MidDensity{ 0.65f };
		float FarDensity{ 0.35f };
		bool Impacts{ true };
		int MaxImpactsPerFrame{ 32 };

		static void Register(sol::table& parent);
	};

	struct WeatherProfile
	{
		WeatherType Type{ WeatherType::None };
		float Strength{ 1.0f };
		bool Clustering{ true };
		WeatherQuality Quality{ WeatherQuality::Auto };
		RainProfile Rain = {};

		static void Register(sol::table& parent);
	};

	struct WindProfile
	{
		float Direction{ 0.0f };
		float Strength{ 0.0f };
		float GustStrength{ 0.0f };
		float GustFrequency{ 0.0f };
		float Turbulence{ 0.0f };
		float VerticalDrift{ 0.0f };

		static void Register(sol::table& parent);
	};

	struct AuroraProfile
	{
		bool Enabled{ false };
		float Intensity{ 0.7f };
		float Speed{ 0.25f };
		float Height{ 0.8f };
		float Width{ 1.0f };
		float WaveScale{ 1.0f };
		float WaveStrength{ 1.0f };
		float Transparency{ 1.0f };
		bool FadeWithFog{ true };

		byte ColorAR{ 80 };
		byte ColorAG{ 180 };
		byte ColorAB{ 255 };
		byte ColorBR{ 120 };
		byte ColorBG{ 255 };
		byte ColorBB{ 180 };
		byte ColorCR{ 180 };
		byte ColorCG{ 120 };
		byte ColorCB{ 255 };

		void SetColorA(Types::ScriptColor const& color);
		void SetColorB(Types::ScriptColor const& color);
		void SetColorC(Types::ScriptColor const& color);
		Types::ScriptColor GetColorA() const;
		Types::ScriptColor GetColorB() const;
		Types::ScriptColor GetColorC() const;

		static void Register(sol::table& parent);
	};

	struct AtmosphereEffectProfile
	{
		bool Enabled{ false };
		AtmosphereEffectType Type{ AtmosphereEffectType::None };
		AtmosphereEffectScope Scope{ AtmosphereEffectScope::Global };
		AtmosphereEffectRenderMode RenderMode{ AtmosphereEffectRenderMode::Generated };
		std::string PresetName = {};
		std::string AnchorName = {};
		std::string TextureName = {};
		float Radius{ 1024.0f };
		float Height{ 512.0f };
		float Density{ 1.0f };
		float Speed{ 0.25f };
		float Direction{ 0.0f };
		float Turbulence{ 0.0f };
		float VerticalDrift{ 0.0f };
		float MinSize{ 16.0f };
		float MaxSize{ 64.0f };
		float Lifetime{ 4.0f };
		float FadeDistance{ 512.0f };
		float Alpha{ 1.0f };
		float GeneratedDetail{ 1.0f };
		float GeneratedSoftness{ 1.0f };
		float GeneratedVariation{ 1.0f };
		int GeneratedSeed{ 0 };
		bool CollideWithGeometry{ true };
		bool StopAtWalls{ true };
		bool StopAtFloors{ true };
		bool StopAtCeilings{ true };
		bool ClampToRoom{ true };
		bool InheritWind{ true };

		byte ColorAR{ 255 };
		byte ColorAG{ 255 };
		byte ColorAB{ 255 };
		byte ColorBR{ 255 };
		byte ColorBG{ 255 };
		byte ColorBB{ 255 };

		void SetColorA(Types::ScriptColor const& color);
		void SetColorB(Types::ScriptColor const& color);
		Types::ScriptColor GetColorA() const;
		Types::ScriptColor GetColorB() const;

		static void Register(sol::table& parent);
	};

	struct AtmosphereRuntimeSnapshot
	{
		bool Enabled{ false };
		WeatherType Type{ WeatherType::None };
		float Strength{ 1.0f };
		bool Clustering{ true };
		WeatherQuality Quality{ WeatherQuality::Auto };
		RainProfile Rain = {};
		WindProfile Wind = {};
		AuroraProfile Aurora = {};
		int EnabledEffectCount{ 0 };
		int LocalEffectCount{ 0 };
	};

	struct Atmosphere
	{
		bool Enabled{ false };
		WeatherProfile Weather = {};
		WindProfile Wind = {};
		AuroraProfile Aurora = {};
		std::vector<AtmosphereEffectProfile> Effects = {};

		AtmosphereRuntimeSnapshot CreateRuntimeSnapshot(WeatherType legacyType, float legacyStrength, bool legacyClustering) const;
		int GetEnabledEffectCount() const;
		int GetLocalEffectCount() const;
		bool HasEnabledEffects() const;
		bool HasLocalEffects() const;

		static void Register(sol::table& parent);
	};

	struct AtmosphereRuntimeController
	{
		AtmosphereRuntimeSnapshot Snapshot = {};

		void Reset();
		void Update(Atmosphere const& atmosphere, WeatherType legacyType, float legacyStrength, bool legacyClustering);
		const AtmosphereRuntimeSnapshot& GetSnapshot() const;
		bool IsEnabled() const;
		bool HasWeather() const;
		bool HasAurora() const;
		bool HasEnabledEffects() const;
		bool HasLocalEffects() const;
	};

	inline AtmosphereRuntimeSnapshot Atmosphere::CreateRuntimeSnapshot(WeatherType legacyType, float legacyStrength, bool legacyClustering) const
	{
		AtmosphereRuntimeSnapshot snapshot = {};
		snapshot.Enabled = Enabled;

		if (!Enabled)
		{
			snapshot.Type = legacyType;
			snapshot.Strength = legacyStrength;
			snapshot.Clustering = legacyClustering;
			return snapshot;
		}

		snapshot.Type = Weather.Type;
		snapshot.Strength = Weather.Strength;
		snapshot.Clustering = Weather.Clustering;
		snapshot.Quality = Weather.Quality;
		snapshot.Rain = Weather.Rain;
		snapshot.Wind = Wind;
		snapshot.Aurora = Aurora;
		snapshot.EnabledEffectCount = GetEnabledEffectCount();
		snapshot.LocalEffectCount = GetLocalEffectCount();
		return snapshot;
	}

	inline int Atmosphere::GetEnabledEffectCount() const
	{
		int count = 0;

		for (auto const& effect : Effects)
		{
			if (effect.Enabled)
				count++;
		}

		return count;
	}

	inline int Atmosphere::GetLocalEffectCount() const
	{
		int count = 0;

		for (auto const& effect : Effects)
		{
			if (effect.Enabled && effect.Scope != AtmosphereEffectScope::Global)
				count++;
		}

		return count;
	}

	inline bool Atmosphere::HasEnabledEffects() const
	{
		return GetEnabledEffectCount() > 0;
	}

	inline bool Atmosphere::HasLocalEffects() const
	{
		return GetLocalEffectCount() > 0;
	}

	inline void AtmosphereRuntimeController::Reset()
	{
		Snapshot = {};
	}

	inline void AtmosphereRuntimeController::Update(Atmosphere const& atmosphere, WeatherType legacyType, float legacyStrength, bool legacyClustering)
	{
		Snapshot = atmosphere.CreateRuntimeSnapshot(legacyType, legacyStrength, legacyClustering);
	}

	inline const AtmosphereRuntimeSnapshot& AtmosphereRuntimeController::GetSnapshot() const
	{
		return Snapshot;
	}

	inline bool AtmosphereRuntimeController::IsEnabled() const
	{
		return Snapshot.Enabled;
	}

	inline bool AtmosphereRuntimeController::HasWeather() const
	{
		return Snapshot.Type != WeatherType::None && Snapshot.Strength > 0.0f;
	}

	inline bool AtmosphereRuntimeController::HasAurora() const
	{
		return Snapshot.Enabled && Snapshot.Aurora.Enabled && Snapshot.Aurora.Intensity > 0.0f;
	}

	inline bool AtmosphereRuntimeController::HasEnabledEffects() const
	{
		return Snapshot.EnabledEffectCount > 0;
	}

	inline bool AtmosphereRuntimeController::HasLocalEffects() const
	{
		return Snapshot.LocalEffectCount > 0;
	}
}