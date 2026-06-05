#pragma once

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

	struct Atmosphere
	{
		bool Enabled{ false };
		WeatherProfile Weather = {};
		WindProfile Wind = {};
		AuroraProfile Aurora = {};

		static void Register(sol::table& parent);
	};
}