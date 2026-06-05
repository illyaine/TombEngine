#pragma once

#include "Specific/RGBAColor8Byte.h"

enum class WeatherType
{
	None,
	Rain,
	Snow
};

enum class LaraType
{
	Normal = 1,
	Young = 2,
	Divesuit = 3
};

class ScriptInterfaceLevel
{
public:
	std::string NameStringKey	   = {};
	std::string FileName		   = {};
	std::string ScriptFileName	   = {};
	std::string LoadScreenFileName = {};

	virtual ~ScriptInterfaceLevel() = default;

	virtual bool GetSkyLayerEnabled(int index) const = 0;
	virtual short GetSkyLayerSpeed(int index) const = 0;
	virtual LaraType GetLaraType() const = 0;
	virtual bool GetStormEnabled() const = 0;
	virtual bool GetRumbleEnabled() const = 0;
	virtual float GetWeatherStrength() const = 0;
	virtual bool GetWeatherClustering() const = 0;
	virtual WeatherType GetWeatherType() const = 0;
	virtual bool GetAtmosphereWeatherEnabled() const { return false; }
	virtual float GetAtmosphereRainWindInfluence() const { return 0.0f; }
	virtual float GetAtmosphereRainNearDensity() const { return 1.0f; }
	virtual float GetAtmosphereRainMidDensity() const { return 0.65f; }
	virtual float GetAtmosphereRainFarDensity() const { return 0.35f; }
	virtual bool GetAtmosphereRainImpactsEnabled() const { return true; }
	virtual int GetAtmosphereRainMaxImpactsPerFrame() const { return 32; }
	virtual bool GetAtmosphereRainScreenDropsEnabled() const { return false; }
	virtual float GetAtmosphereRainScreenDropDensity() const { return 0.0f; }
	virtual float GetAtmosphereRainScreenDropFadeSpeed() const { return 0.05f; }
	virtual float GetAtmosphereWindDirection() const { return 0.0f; }
	virtual float GetAtmosphereWindStrength() const { return 0.0f; }
	virtual float GetAtmosphereWindGustStrength() const { return 0.0f; }
	virtual float GetAtmosphereWindGustFrequency() const { return 0.0f; }
	virtual float GetAtmosphereWindTurbulence() const { return 0.0f; }
	virtual float GetAtmosphereWindVerticalDrift() const { return 0.0f; }
	virtual RGBAColor8Byte GetSkyLayerColor(int index) const = 0;
	virtual RGBAColor8Byte GetFogColor() const = 0;
	virtual float GetFogMinDistance() const = 0;
	virtual float GetFogMaxDistance() const = 0;
	virtual float GetFarView() const = 0;
	virtual int GetSecrets() const = 0;
	virtual std::string GetAmbientTrack() const = 0;
	virtual bool GetResetHubEnabled() const = 0;

	// Horizon getters
	virtual bool		   GetHorizonEnabled(int index) const = 0;
	virtual GAME_OBJECT_ID GetHorizonObjectID(int index) const = 0;
	virtual float		   GetHorizonTransparency(int index) const = 0;
	virtual Vector3		   GetHorizonPosition(int index) const = 0;
	virtual EulerAngles	   GetHorizonOrientation(int index) const = 0;
	virtual Vector3		   GetHorizonPrevPosition(int index) const = 0;
	virtual EulerAngles	   GetHorizonPrevOrientation(int index) const = 0;

	// Lens flare getters
	virtual bool  GetLensFlareEnabled() const = 0;
	virtual int	  GetLensFlareSunSpriteID() const = 0;
	virtual short GetLensFlarePitch() const = 0;
	virtual short GetLensFlareYaw() const = 0;
	virtual Color GetLensFlareColor() const = 0;

	// Starfield getters
	virtual int	  GetStarfieldStarCount() const = 0;
	virtual int	  GetStarfieldMeteorCount() const = 0;
	virtual int	  GetStarfieldMeteorSpawnDensity() const = 0;
	virtual float GetStarfieldMeteorVelocity() const = 0;
};