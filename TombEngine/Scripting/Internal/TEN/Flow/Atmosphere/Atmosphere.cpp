#include "framework.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/Atmosphere.h"

using namespace TEN::Scripting;
using namespace TEN::Scripting::Types;

/*** Rain weather settings. To be used with @{Flow.WeatherProfile.rain}.
@tenprimitive Flow.RainProfile
@pragma nostrip
*/
void RainProfile::Register(sol::table& parent)
{
	using ctors = sol::constructors<RainProfile()>;
	parent.new_usertype<RainProfile>("RainProfile",
		ctors(),
		sol::call_constructor, ctors(),

		/// (float) How much wind affects rain direction.
		//@mem windInfluence
		"windInfluence", &RainProfile::WindInfluence,

		/// (float) Near weather density multiplier.
		//@mem nearDensity
		"nearDensity", &RainProfile::NearDensity,

		/// (float) Mid-range weather density multiplier.
		//@mem midDensity
		"midDensity", &RainProfile::MidDensity,

		/// (float) Far weather density multiplier.
		//@mem farDensity
		"farDensity", &RainProfile::FarDensity,

		/// (bool) Enable rain impact effects.
		//@mem impacts
		"impacts", &RainProfile::Impacts,

		/// (int) Maximum rain impact events per frame.
		//@mem maxImpactsPerFrame
		"maxImpactsPerFrame", &RainProfile::MaxImpactsPerFrame
	);
}

/*** Weather profile settings. To be used with @{Flow.Atmosphere.weather}.
@tenprimitive Flow.WeatherProfile
@pragma nostrip
*/
void WeatherProfile::Register(sol::table& parent)
{
	using ctors = sol::constructors<WeatherProfile()>;
	parent.new_usertype<WeatherProfile>("WeatherProfile",
		ctors(),
		sol::call_constructor, ctors(),

		/// (WeatherType) Weather type.
		//@mem type
		"type", &WeatherProfile::Type,

		/// (float) Weather strength.
		//@mem strength
		"strength", &WeatherProfile::Strength,

		/// (bool) Use clustered weather particles.
		//@mem clustering
		"clustering", &WeatherProfile::Clustering,

		/// (WeatherQuality) Weather quality budget.
		//@mem quality
		"quality", &WeatherProfile::Quality,

		/// (@{Flow.RainProfile}) Rain-specific settings.
		//@mem rain
		"rain", &WeatherProfile::Rain
	);
}

/*** Wind profile settings. To be used with @{Flow.Atmosphere.wind}.
@tenprimitive Flow.WindProfile
@pragma nostrip
*/
void WindProfile::Register(sol::table& parent)
{
	using ctors = sol::constructors<WindProfile()>;
	parent.new_usertype<WindProfile>("WindProfile",
		ctors(),
		sol::call_constructor, ctors(),

		/// (float) Wind direction in degrees.
		//@mem direction
		"direction", &WindProfile::Direction,

		/// (float) Base wind strength.
		//@mem strength
		"strength", &WindProfile::Strength,

		/// (float) Gust strength.
		//@mem gustStrength
		"gustStrength", &WindProfile::GustStrength,

		/// (float) Gust frequency.
		//@mem gustFrequency
		"gustFrequency", &WindProfile::GustFrequency,

		/// (float) Wind turbulence.
		//@mem turbulence
		"turbulence", &WindProfile::Turbulence,

		/// (float) Vertical wind drift.
		//@mem verticalDrift
		"verticalDrift", &WindProfile::VerticalDrift
	);
}

/*** Aurora sky effect settings. To be used with @{Flow.Atmosphere.aurora}.
@tenprimitive Flow.AuroraProfile
@pragma nostrip
*/
void AuroraProfile::Register(sol::table& parent)
{
	using ctors = sol::constructors<AuroraProfile()>;
	parent.new_usertype<AuroraProfile>("AuroraProfile",
		ctors(),
		sol::call_constructor, ctors(),

		/// (bool) Enable aurora sky effect.
		//@mem enabled
		"enabled", &AuroraProfile::Enabled,

		/// (float) Aurora intensity.
		//@mem intensity
		"intensity", &AuroraProfile::Intensity,

		/// (float) Aurora animation speed.
		//@mem speed
		"speed", &AuroraProfile::Speed,

		/// (float) Aurora height in sky space.
		//@mem height
		"height", &AuroraProfile::Height,

		/// (float) Aurora width in sky space.
		//@mem width
		"width", &AuroraProfile::Width,

		/// (float) Aurora wave scale.
		//@mem waveScale
		"waveScale", &AuroraProfile::WaveScale,

		/// (float) Aurora wave strength.
		//@mem waveStrength
		"waveStrength", &AuroraProfile::WaveStrength,

		/// (@{Color}) First aurora color.
		//@mem colorA
		"colorA", sol::property(&AuroraProfile::GetColorA, &AuroraProfile::SetColorA),

		/// (@{Color}) Second aurora color.
		//@mem colorB
		"colorB", sol::property(&AuroraProfile::GetColorB, &AuroraProfile::SetColorB),

		/// (@{Color}) Third aurora color.
		//@mem colorC
		"colorC", sol::property(&AuroraProfile::GetColorC, &AuroraProfile::SetColorC),

		/// (float) Aurora transparency.
		//@mem transparency
		"transparency", &AuroraProfile::Transparency,

		/// (bool) Fade aurora with level fog.
		//@mem fadeWithFog
		"fadeWithFog", &AuroraProfile::FadeWithFog
	);
}

/*** Atmosphere settings. To be used with @{Flow.Level.atmosphere}.
@tenprimitive Flow.Atmosphere
@pragma nostrip
*/
void Atmosphere::Register(sol::table& parent)
{
	using ctors = sol::constructors<Atmosphere()>;
	parent.new_usertype<Atmosphere>("Atmosphere",
		ctors(),
		sol::call_constructor, ctors(),

		/// (bool) Enable atmosphere profile processing.
		//@mem enabled
		"enabled", &Atmosphere::Enabled,

		/// (@{Flow.WeatherProfile}) Weather settings.
		//@mem weather
		"weather", &Atmosphere::Weather,

		/// (@{Flow.WindProfile}) Wind settings.
		//@mem wind
		"wind", &Atmosphere::Wind,

		/// (@{Flow.AuroraProfile}) Aurora sky effect settings.
		//@mem aurora
		"aurora", &Atmosphere::Aurora
	);
}

void AuroraProfile::SetColorA(ScriptColor const& color)
{
	ColorAR = color.GetR();
	ColorAG = color.GetG();
	ColorAB = color.GetB();
}

void AuroraProfile::SetColorB(ScriptColor const& color)
{
	ColorBR = color.GetR();
	ColorBG = color.GetG();
	ColorBB = color.GetB();
}

void AuroraProfile::SetColorC(ScriptColor const& color)
{
	ColorCR = color.GetR();
	ColorCG = color.GetG();
	ColorCB = color.GetB();
}

ScriptColor AuroraProfile::GetColorA() const
{
	return ScriptColor{ ColorAR, ColorAG, ColorAB };
}

ScriptColor AuroraProfile::GetColorB() const
{
	return ScriptColor{ ColorBR, ColorBG, ColorBB };
}

ScriptColor AuroraProfile::GetColorC() const
{
	return ScriptColor{ ColorCR, ColorCG, ColorCB };
}
