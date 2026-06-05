#pragma once

#include <string>
#include <unordered_map>

#include "Scripting/Include/ScriptInterfaceLevel.h"

namespace TEN::Scripting
{
	enum class AtmosphereEnvironmentMode
	{
		EarthLike,
		Indoor,
		Underground,
		Underwater,
		SpaceVacuum,
		AlienPlanet,
		ToxicPlanet,
		ArtificialDome,
		Fantasy,
		Custom
	};

	enum class AtmospherePrecipitationMaterial
	{
		Water,
		Snow,
		Ash,
		Sand,
		Acid,
		ToxicFluid,
		Methane,
		Spores,
		CrystalDust,
		RadiationDust,
		Plasma,
		MagicEnergy,
		Custom
	};

	/// Constants for atmosphere environment modes.
	// @enum Flow.AtmosphereEnvironmentMode
	// @pragma nostrip
	static const std::unordered_map<std::string, AtmosphereEnvironmentMode> ATMOSPHERE_ENVIRONMENT_MODES
	{
		/// Earth-like outdoor atmosphere.
		// @mem EarthLike
		{ "EarthLike", AtmosphereEnvironmentMode::EarthLike },

		/// Indoor atmosphere.
		// @mem Indoor
		{ "Indoor", AtmosphereEnvironmentMode::Indoor },

		/// Underground atmosphere.
		// @mem Underground
		{ "Underground", AtmosphereEnvironmentMode::Underground },

		/// Underwater or fully flooded environment.
		// @mem Underwater
		{ "Underwater", AtmosphereEnvironmentMode::Underwater },

		/// Space or vacuum environment.
		// @mem SpaceVacuum
		{ "SpaceVacuum", AtmosphereEnvironmentMode::SpaceVacuum },

		/// Alien planet with non-standard atmosphere.
		// @mem AlienPlanet
		{ "AlienPlanet", AtmosphereEnvironmentMode::AlienPlanet },

		/// Toxic planet with hostile atmosphere defaults.
		// @mem ToxicPlanet
		{ "ToxicPlanet", AtmosphereEnvironmentMode::ToxicPlanet },

		/// Artificial dome or controlled habitat.
		// @mem ArtificialDome
		{ "ArtificialDome", AtmosphereEnvironmentMode::ArtificialDome },

		/// Fantasy atmosphere where normal physical rules can be bent.
		// @mem Fantasy
		{ "Fantasy", AtmosphereEnvironmentMode::Fantasy },

		/// Custom environment mode.
		// @mem Custom
		{ "Custom", AtmosphereEnvironmentMode::Custom }
	};

	/// Constants for atmosphere precipitation materials.
	// @enum Flow.AtmospherePrecipitationMaterial
	// @pragma nostrip
	static const std::unordered_map<std::string, AtmospherePrecipitationMaterial> ATMOSPHERE_PRECIPITATION_MATERIALS
	{
		/// Normal water rain.
		// @mem Water
		{ "Water", AtmospherePrecipitationMaterial::Water },

		/// Normal snow.
		// @mem Snow
		{ "Snow", AtmospherePrecipitationMaterial::Snow },

		/// Falling ash.
		// @mem Ash
		{ "Ash", AtmospherePrecipitationMaterial::Ash },

		/// Sand or dust precipitation.
		// @mem Sand
		{ "Sand", AtmospherePrecipitationMaterial::Sand },

		/// Acid precipitation.
		// @mem Acid
		{ "Acid", AtmospherePrecipitationMaterial::Acid },

		/// Toxic liquid precipitation.
		// @mem ToxicFluid
		{ "ToxicFluid", AtmospherePrecipitationMaterial::ToxicFluid },

		/// Methane or similar alien precipitation.
		// @mem Methane
		{ "Methane", AtmospherePrecipitationMaterial::Methane },

		/// Organic spores.
		// @mem Spores
		{ "Spores", AtmospherePrecipitationMaterial::Spores },

		/// Crystal dust precipitation.
		// @mem CrystalDust
		{ "CrystalDust", AtmospherePrecipitationMaterial::CrystalDust },

		/// Radioactive or hazardous dust.
		// @mem RadiationDust
		{ "RadiationDust", AtmospherePrecipitationMaterial::RadiationDust },

		/// Plasma precipitation.
		// @mem Plasma
		{ "Plasma", AtmospherePrecipitationMaterial::Plasma },

		/// Magic or energy precipitation.
		// @mem MagicEnergy
		{ "MagicEnergy", AtmospherePrecipitationMaterial::MagicEnergy },

		/// Custom precipitation material.
		// @mem Custom
		{ "Custom", AtmospherePrecipitationMaterial::Custom }
	};

	struct AtmosphereEnvironmentProfile
	{
		bool Enabled{ false };
		AtmosphereEnvironmentMode Mode{ AtmosphereEnvironmentMode::EarthLike };
		AtmospherePrecipitationMaterial PrecipitationMaterial{ AtmospherePrecipitationMaterial::Water };
		bool ArtificialWeather{ false };
		bool AllowToxicWeather{ false };
		bool ForceAllowPrecipitation{ false };
		bool ForceAllowWind{ false };
		bool VisualOnlyHazards{ true };
		float DamagePerSecond{ 0.0f };

		bool IsSpaceMode() const;
		bool IsAlienOrToxicMode() const;
		bool IsArtificialEnvironment() const;
		bool AllowsWeatherType(WeatherType type) const;
		bool AllowsNormalPrecipitation() const;
		bool AllowsHazardousPrecipitation() const;
		bool AllowsWind() const;
		bool HasHazardousPrecipitation() const;
		bool HasGameplayHazard() const;
		float GetHazardDamagePerSecond() const;

		static void Register(sol::table& parent);
	};

	inline bool AtmosphereEnvironmentProfile::IsSpaceMode() const
	{
		return Mode == AtmosphereEnvironmentMode::SpaceVacuum;
	}

	inline bool AtmosphereEnvironmentProfile::IsAlienOrToxicMode() const
	{
		return Mode == AtmosphereEnvironmentMode::AlienPlanet || Mode == AtmosphereEnvironmentMode::ToxicPlanet;
	}

	inline bool AtmosphereEnvironmentProfile::IsArtificialEnvironment() const
	{
		return Mode == AtmosphereEnvironmentMode::ArtificialDome || ArtificialWeather;
	}

	inline bool AtmosphereEnvironmentProfile::AllowsWeatherType(WeatherType type) const
	{
		if (!Enabled || type == WeatherType::None)
			return true;

		if (ForceAllowPrecipitation)
			return true;

		switch (Mode)
		{
		case AtmosphereEnvironmentMode::SpaceVacuum:
		case AtmosphereEnvironmentMode::Underwater:
		case AtmosphereEnvironmentMode::Indoor:
		case AtmosphereEnvironmentMode::Underground:
			return ArtificialWeather;

		default:
			return true;
		}
	}

	inline bool AtmosphereEnvironmentProfile::AllowsNormalPrecipitation() const
	{
		if (!Enabled)
			return true;

		if (!AllowsWeatherType(WeatherType::Rain))
			return false;

		switch (PrecipitationMaterial)
		{
		case AtmospherePrecipitationMaterial::Water:
		case AtmospherePrecipitationMaterial::Snow:
			return true;

		default:
			return false;
		}
	}

	inline bool AtmosphereEnvironmentProfile::AllowsHazardousPrecipitation() const
	{
		if (!Enabled || !HasHazardousPrecipitation())
			return true;

		return AllowToxicWeather || ForceAllowPrecipitation || IsAlienOrToxicMode() || Mode == AtmosphereEnvironmentMode::Fantasy || Mode == AtmosphereEnvironmentMode::Custom;
	}

	inline bool AtmosphereEnvironmentProfile::AllowsWind() const
	{
		if (!Enabled || ForceAllowWind)
			return true;

		if (Mode == AtmosphereEnvironmentMode::SpaceVacuum)
			return ArtificialWeather;

		return true;
	}

	inline bool AtmosphereEnvironmentProfile::HasHazardousPrecipitation() const
	{
		switch (PrecipitationMaterial)
		{
		case AtmospherePrecipitationMaterial::Acid:
		case AtmospherePrecipitationMaterial::ToxicFluid:
		case AtmospherePrecipitationMaterial::RadiationDust:
		case AtmospherePrecipitationMaterial::Plasma:
			return true;

		default:
			return false;
		}
	}

	inline bool AtmosphereEnvironmentProfile::HasGameplayHazard() const
	{
		return Enabled && AllowsHazardousPrecipitation() && !VisualOnlyHazards && GetHazardDamagePerSecond() > 0.0f;
	}

	inline float AtmosphereEnvironmentProfile::GetHazardDamagePerSecond() const
	{
		if (!Enabled || !HasHazardousPrecipitation())
			return 0.0f;

		return DamagePerSecond > 0.0f ? DamagePerSecond : 0.0f;
	}

	/*** Environment rules for atmosphere and planetary weather. To be used with @{Flow.Level.atmosphereEnvironment}.
	@tenprimitive Flow.AtmosphereEnvironmentProfile
	@pragma nostrip
	*/
	inline void AtmosphereEnvironmentProfile::Register(sol::table& parent)
	{
		parent.set("AtmosphereEnvironmentMode", ATMOSPHERE_ENVIRONMENT_MODES);
		parent.set("AtmospherePrecipitationMaterial", ATMOSPHERE_PRECIPITATION_MATERIALS);

		using ctors = sol::constructors<AtmosphereEnvironmentProfile()>;
		parent.new_usertype<AtmosphereEnvironmentProfile>("AtmosphereEnvironmentProfile",
			ctors(),
			sol::call_constructor, ctors(),

			/// (bool) Enable environment rules for weather and wind.
			//@mem enabled
			"enabled", &AtmosphereEnvironmentProfile::Enabled,

			/// (AtmosphereEnvironmentMode) Select the broad environment mode.
			//@mem mode
			"mode", &AtmosphereEnvironmentProfile::Mode,

			/// (AtmospherePrecipitationMaterial) Select what precipitation represents in this environment.
			//@mem precipitationMaterial
			"precipitationMaterial", &AtmosphereEnvironmentProfile::PrecipitationMaterial,

			/// (bool) Allow artificial weather in locations where natural weather would normally be disabled.
			//@mem artificialWeather
			"artificialWeather", &AtmosphereEnvironmentProfile::ArtificialWeather,

			/// (bool) Mark hazardous precipitation as intentionally allowed for this environment.
			//@mem allowToxicWeather
			"allowToxicWeather", &AtmosphereEnvironmentProfile::AllowToxicWeather,

			/// (bool) Force precipitation to stay available even in restricted environment modes.
			//@mem forceAllowPrecipitation
			"forceAllowPrecipitation", &AtmosphereEnvironmentProfile::ForceAllowPrecipitation,

			/// (bool) Force wind to stay available even in restricted environment modes.
			//@mem forceAllowWind
			"forceAllowWind", &AtmosphereEnvironmentProfile::ForceAllowWind,

			/// (bool) Treat hazardous precipitation as visual-only unless gameplay code explicitly uses it.
			//@mem visualOnlyHazards
			"visualOnlyHazards", &AtmosphereEnvironmentProfile::VisualOnlyHazards,

			/// (float) Optional damage amount for hazardous precipitation when gameplay code uses this profile.
			//@mem damagePerSecond
			"damagePerSecond", &AtmosphereEnvironmentProfile::DamagePerSecond,

			/// (function) Returns true when this profile uses a space or vacuum mode.
			//@mem isSpaceMode
			"isSpaceMode", &AtmosphereEnvironmentProfile::IsSpaceMode,

			/// (function) Returns true for alien or toxic planet modes.
			//@mem isAlienOrToxicMode
			"isAlienOrToxicMode", &AtmosphereEnvironmentProfile::IsAlienOrToxicMode,

			/// (function) Returns true for artificial atmosphere handling.
			//@mem isArtificialEnvironment
			"isArtificialEnvironment", &AtmosphereEnvironmentProfile::IsArtificialEnvironment,

			/// (function) Returns whether a weather type is allowed in this environment.
			//@mem allowsWeatherType
			"allowsWeatherType", &AtmosphereEnvironmentProfile::AllowsWeatherType,

			/// (function) Returns whether water or snow precipitation is still considered normal here.
			//@mem allowsNormalPrecipitation
			"allowsNormalPrecipitation", &AtmosphereEnvironmentProfile::AllowsNormalPrecipitation,

			/// (function) Returns whether hazardous precipitation is allowed in this environment.
			//@mem allowsHazardousPrecipitation
			"allowsHazardousPrecipitation", &AtmosphereEnvironmentProfile::AllowsHazardousPrecipitation,

			/// (function) Returns whether wind is allowed in this environment.
			//@mem allowsWind
			"allowsWind", &AtmosphereEnvironmentProfile::AllowsWind,

			/// (function) Returns whether the selected precipitation material is hazardous.
			//@mem hasHazardousPrecipitation
			"hasHazardousPrecipitation", &AtmosphereEnvironmentProfile::HasHazardousPrecipitation,

			/// (function) Returns whether hazardous precipitation should currently be treated as gameplay damage.
			//@mem hasGameplayHazard
			"hasGameplayHazard", &AtmosphereEnvironmentProfile::HasGameplayHazard,

			/// (function) Returns the effective damage amount for hazardous precipitation.
			//@mem getHazardDamagePerSecond
			"getHazardDamagePerSecond", &AtmosphereEnvironmentProfile::GetHazardDamagePerSecond
		);
	}
}
