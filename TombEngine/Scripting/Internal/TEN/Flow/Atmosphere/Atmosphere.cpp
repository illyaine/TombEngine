#include "framework.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/Atmosphere.h"

using namespace TEN::Scripting;
using namespace TEN::Scripting::Types;

/** Rain weather settings. To be used with @{Flow.WeatherProfile.rain}.
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

/** Weather profile settings. To be used with @{Flow.Atmosphere.weather}.
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

/** Wind profile settings. To be used with @{Flow.Atmosphere.wind}.
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

/** Aurora sky effect settings. To be used with @{Flow.Atmosphere.aurora}.
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

/** Moon sky object settings. To be used with @{Flow.Atmosphere.moon}.
@tenprimitive Flow.MoonProfile
@pragma nostrip
*/
void MoonProfile::Register(sol::table& parent)
{
	using ctors = sol::constructors<MoonProfile()>;
	parent.new_usertype<MoonProfile>("MoonProfile",
		ctors(),
		sol::call_constructor, ctors(),

		/// (bool) Enable moon sky object.
		//@mem enabled
		"enabled", &MoonProfile::Enabled,

		/// (float) Moon pitch angle in degrees.
		//@mem pitch
		"pitch", &MoonProfile::Pitch,

		/// (float) Moon yaw angle in degrees.
		//@mem yaw
		"yaw", &MoonProfile::Yaw,

		/// (float) Relative moon size.
		//@mem size
		"size", &MoonProfile::Size,

		/// (float) Visible moon brightness.
		//@mem intensity
		"intensity", &MoonProfile::Intensity,

		/// (float) Moon halo brightness.
		//@mem haloIntensity
		"haloIntensity", &MoonProfile::HaloIntensity,

		/// (float) Global moon light influence.
		//@mem lightIntensity
		"lightIntensity", &MoonProfile::LightIntensity,

		/// (float) Moon phase from 0.0 to 1.0.
		//@mem phase
		"phase", &MoonProfile::Phase,

		/// (bool) Fade moon with fog.
		//@mem fadeWithFog
		"fadeWithFog", &MoonProfile::FadeWithFog,

		/// (bool) Allow moon direction to drive matching light shafts.
		//@mem drivesLightShafts
		"drivesLightShafts", &MoonProfile::DrivesLightShafts,

		/// (string) Optional moon texture name.
		//@mem textureName
		"textureName", &MoonProfile::TextureName,

		/// (@{Color}) Visible moon color.
		//@mem color
		"color", sol::property(&MoonProfile::GetColor, &MoonProfile::SetColor),

		/// (@{Color}) Moonlight color.
		//@mem lightColor
		"lightColor", sol::property(&MoonProfile::GetLightColor, &MoonProfile::SetLightColor)
	);
}

/** Data for one generated or anchored atmosphere effect layer. To be used with @{Flow.Atmosphere.effects}.
@tenprimitive Flow.AtmosphereEffectProfile
@pragma nostrip
*/
void AtmosphereEffectProfile::Register(sol::table& parent)
{
	using ctors = sol::constructors<AtmosphereEffectProfile()>;
	parent.new_usertype<AtmosphereEffectProfile>("AtmosphereEffectProfile",
		ctors(),
		sol::call_constructor, ctors(),

		/// (bool) Enable this effect layer.
		//@mem enabled
		"enabled", &AtmosphereEffectProfile::Enabled,

		/// (AtmosphereEffectType) Select the base effect type, such as leaf fall, ground fog, dust, ash, or a custom preset.
		//@mem type
		"type", &AtmosphereEffectProfile::Type,

		/// (AtmosphereEffectScope) Select whether this effect is global or anchored to a nullmesh, room, or volume.
		//@mem scope
		"scope", &AtmosphereEffectProfile::Scope,

		/// (AtmosphereEffectRenderMode) Choose generated rendering, an optional sprite texture, a bridge to an existing effect, or a custom renderer later.
		//@mem renderMode
		"renderMode", &AtmosphereEffectProfile::RenderMode,

		/// (string) Optional preset name for generated or custom atmosphere effects.
		//@mem presetName
		"presetName", &AtmosphereEffectProfile::PresetName,

		/// (string) Optional nullmesh or object name used as emitter or anchor when scope is Nullmesh.
		//@mem anchorName
		"anchorName", &AtmosphereEffectProfile::AnchorName,

		/// (string) Optional texture name used only when renderMode is Sprite or a custom path explicitly asks for it.
		//@mem textureName
		"textureName", &AtmosphereEffectProfile::TextureName,

		/// (float) Horizontal influence radius around the effect source.
		//@mem radius
		"radius", &AtmosphereEffectProfile::Radius,

		/// (float) Vertical effect height.
		//@mem height
		"height", &AtmosphereEffectProfile::Height,

		/// (float) Density multiplier for generated particles, sheets, or volume slices.
		//@mem density
		"density", &AtmosphereEffectProfile::Density,

		/// (float) Movement or animation speed.
		//@mem speed
		"speed", &AtmosphereEffectProfile::Speed,

		/// (float) Movement direction in degrees.
		//@mem direction
		"direction", &AtmosphereEffectProfile::Direction,

		/// (float) Local turbulence multiplier.
		//@mem turbulence
		"turbulence", &AtmosphereEffectProfile::Turbulence,

		/// (float) Vertical drift multiplier.
		//@mem verticalDrift
		"verticalDrift", &AtmosphereEffectProfile::VerticalDrift,

		/// (float) Minimum generated element size.
		//@mem minSize
		"minSize", &AtmosphereEffectProfile::MinSize,

		/// (float) Maximum generated element size.
		//@mem maxSize
		"maxSize", &AtmosphereEffectProfile::MaxSize,

		/// (float) Lifetime for generated moving elements.
		//@mem lifetime
		"lifetime", &AtmosphereEffectProfile::Lifetime,

		/// (float) Distance used to fade the effect in or out near limits.
		//@mem fadeDistance
		"fadeDistance", &AtmosphereEffectProfile::FadeDistance,

		/// (float) Alpha multiplier.
		//@mem alpha
		"alpha", &AtmosphereEffectProfile::Alpha,

		/// (float) Detail amount for generated noise, shapes, or internal variation.
		//@mem generatedDetail
		"generatedDetail", &AtmosphereEffectProfile::GeneratedDetail,

		/// (float) Softness for generated layer edges or volume impression.
		//@mem generatedSoftness
		"generatedSoftness", &AtmosphereEffectProfile::GeneratedSoftness,

		/// (float) Variation amount for generated shapes.
		//@mem generatedVariation
		"generatedVariation", &AtmosphereEffectProfile::GeneratedVariation,

		/// (int) Seed for deterministic generated effect variation. Zero lets the engine choose a stable default.
		//@mem generatedSeed
		"generatedSeed", &AtmosphereEffectProfile::GeneratedSeed,

		/// (bool) Test generated movement against level geometry.
		//@mem collideWithGeometry
		"collideWithGeometry", &AtmosphereEffectProfile::CollideWithGeometry,

		/// (bool) Stop generated movement at walls instead of passing through them.
		//@mem stopAtWalls
		"stopAtWalls", &AtmosphereEffectProfile::StopAtWalls,

		/// (bool) Stop generated movement at floors instead of passing through them.
		//@mem stopAtFloors
		"stopAtFloors", &AtmosphereEffectProfile::StopAtFloors,

		/// (bool) Stop generated movement at ceilings instead of passing through them.
		//@mem stopAtCeilings
		"stopAtCeilings", &AtmosphereEffectProfile::StopAtCeilings,

		/// (bool) Keep the generated effect inside the active room or anchored room where possible.
		//@mem clampToRoom
		"clampToRoom", &AtmosphereEffectProfile::ClampToRoom,

		/// (bool) Add the global wind profile to this effect's local movement.
		//@mem inheritWind
		"inheritWind", &AtmosphereEffectProfile::InheritWind,

		/// (@{Color}) Primary generated color.
		//@mem colorA
		"colorA", sol::property(&AtmosphereEffectProfile::GetColorA, &AtmosphereEffectProfile::SetColorA),

		/// (@{Color}) Secondary generated color.
		//@mem colorB
		"colorB", sol::property(&AtmosphereEffectProfile::GetColorB, &AtmosphereEffectProfile::SetColorB)
	);
}

/** Data for one global or anchored light shaft. To be used with @{Flow.Atmosphere.lightShafts}.
@tenprimitive Flow.LightShaftProfile
@pragma nostrip
*/
void LightShaftProfile::Register(sol::table& parent)
{
	using ctors = sol::constructors<LightShaftProfile()>;
	parent.new_usertype<LightShaftProfile>("LightShaftProfile",
		ctors(),
		sol::call_constructor, ctors(),

		/// (bool) Enable this light shaft.
		//@mem enabled
		"enabled", &LightShaftProfile::Enabled,

		/// (AtmosphereEffectScope) Select global, nullmesh, room, or volume scope.
		//@mem scope
		"scope", &LightShaftProfile::Scope,

		/// (string) Optional nullmesh or object name used as source or anchor.
		//@mem anchorName
		"anchorName", &LightShaftProfile::AnchorName,

		/// (float) Light shaft pitch angle in degrees.
		//@mem pitch
		"pitch", &LightShaftProfile::Pitch,

		/// (float) Light shaft yaw angle in degrees.
		//@mem yaw
		"yaw", &LightShaftProfile::Yaw,

		/// (float) Light shaft length.
		//@mem length
		"length", &LightShaftProfile::Length,

		/// (float) Light shaft radius.
		//@mem radius
		"radius", &LightShaftProfile::Radius,

		/// (float) Light shaft brightness.
		//@mem intensity
		"intensity", &LightShaftProfile::Intensity,

		/// (float) Internal volumetric density.
		//@mem density
		"density", &LightShaftProfile::Density,

		/// (float) Edge softness.
		//@mem softness
		"softness", &LightShaftProfile::Softness,

		/// (float) Dust shimmer density inside the shaft.
		//@mem dustDensity
		"dustDensity", &LightShaftProfile::DustDensity,

		/// (bool) Use the moon direction instead of this profile's pitch/yaw.
		//@mem inheritMoonDirection
		"inheritMoonDirection", &LightShaftProfile::InheritMoonDirection,

		/// (bool) Fade with fog.
		//@mem fadeWithFog
		"fadeWithFog", &LightShaftProfile::FadeWithFog,

		/// (bool) Allow geometry to block the shaft.
		//@mem blockedByGeometry
		"blockedByGeometry", &LightShaftProfile::BlockedByGeometry,

		/// (bool) Keep local shafts inside the anchored room where possible.
		//@mem clampToRoom
		"clampToRoom", &LightShaftProfile::ClampToRoom,

		/// (@{Color}) Light shaft color.
		//@mem color
		"color", sol::property(&LightShaftProfile::GetColor, &LightShaftProfile::SetColor)
	);
}

/** Atmosphere settings. To be used with @{Flow.Level.atmosphere}.
@tenprimitive Flow.Atmosphere
@pragma nostrip
*/
void Atmosphere::Register(sol::table& parent)
{
	parent.set("WeatherQuality", WEATHER_QUALITIES);
	parent.set("AtmosphereEffectType", ATMOSPHERE_EFFECT_TYPES);
	parent.set("AtmosphereEffectScope", ATMOSPHERE_EFFECT_SCOPES);
	parent.set("AtmosphereEffectRenderMode", ATMOSPHERE_EFFECT_RENDER_MODES);

	RainProfile::Register(parent);
	WeatherProfile::Register(parent);
	WindProfile::Register(parent);
	AuroraProfile::Register(parent);
	MoonProfile::Register(parent);
	AtmosphereEffectProfile::Register(parent);
	LightShaftProfile::Register(parent);

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
		"aurora", &Atmosphere::Aurora,

		/// (@{Flow.MoonProfile}) Moon sky object settings.
		//@mem moon
		"moon", &Atmosphere::Moon,

		/// (@{Flow.AtmosphereEffectProfile}[]) Generated, anchored, or custom atmosphere effect layers.
		//@mem effects
		"effects", &Atmosphere::Effects,

		/// (@{Flow.LightShaftProfile}[]) Global or anchored light shafts.
		//@mem lightShafts
		"lightShafts", &Atmosphere::LightShafts
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
	return RGBAColor8Byte(ColorAR, ColorAG, ColorAB);
}

ScriptColor AuroraProfile::GetColorB() const
{
	return RGBAColor8Byte(ColorBR, ColorBG, ColorBB);
}

ScriptColor AuroraProfile::GetColorC() const
{
	return RGBAColor8Byte(ColorCR, ColorCG, ColorCB);
}

void MoonProfile::SetColor(ScriptColor const& color)
{
	ColorR = color.GetR();
	ColorG = color.GetG();
	ColorB = color.GetB();
}

void MoonProfile::SetLightColor(ScriptColor const& color)
{
	LightColorR = color.GetR();
	LightColorG = color.GetG();
	LightColorB = color.GetB();
}

ScriptColor MoonProfile::GetColor() const
{
	return RGBAColor8Byte(ColorR, ColorG, ColorB);
}

ScriptColor MoonProfile::GetLightColor() const
{
	return RGBAColor8Byte(LightColorR, LightColorG, LightColorB);
}

void AtmosphereEffectProfile::SetColorA(ScriptColor const& color)
{
	ColorAR = color.GetR();
	ColorAG = color.GetG();
	ColorAB = color.GetB();
}

void AtmosphereEffectProfile::SetColorB(ScriptColor const& color)
{
	ColorBR = color.GetR();
	ColorBG = color.GetG();
	ColorBB = color.GetB();
}

ScriptColor AtmosphereEffectProfile::GetColorA() const
{
	return RGBAColor8Byte(ColorAR, ColorAG, ColorAB);
}

ScriptColor AtmosphereEffectProfile::GetColorB() const
{
	return RGBAColor8Byte(ColorBR, ColorBG, ColorBB);
}

void LightShaftProfile::SetColor(ScriptColor const& color)
{
	ColorR = color.GetR();
	ColorG = color.GetG();
	ColorB = color.GetB();
}

ScriptColor LightShaftProfile::GetColor() const
{
	return RGBAColor8Byte(ColorR, ColorG, ColorB);
}
