#pragma once
#include "Scripting/Internal/TEN/Flow/Atmosphere/Atmosphere.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereCelestial.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereCelestialRender.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereEnvironment.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereEnvironmentRuntime.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereRuntimeState.h"
#include "Scripting/Internal/TEN/Flow/Horizon/Horizon.h"
#include "Scripting/Internal/TEN/Flow/LensFlare/LensFlare.h"
#include "Scripting/Internal/TEN/Flow/SkyLayer/SkyLayer.h"
#include "Scripting/Internal/TEN/Flow/Starfield/Starfield.h"
#include "Scripting/Internal/TEN/Flow/Fog/Fog.h"
#include "Scripting/Include/ScriptInterfaceLevel.h"
#include "Scripting/Internal/TEN/Flow/InventoryItem/InventoryItem.h"

using namespace TEN::Scripting;

struct SkyAtmosphereRenderData
{
	bool Layer1Enabled{ false };
	bool Layer2Enabled{ false };
	bool Horizon1Enabled{ false };
	bool Horizon2Enabled{ false };
	bool LensFlareEnabled{ false };
	bool StarfieldEnabled{ false };
	bool StormEnabled{ false };
	bool LegacySkyEnabled{ false };
	bool AtmosphereCelestialEnabled{ false };
	int AtmosphereCelestialBodyCount{ 0 };
	TEN::Scripting::AtmosphereRenderData AtmosphereData = {};
	TEN::Scripting::AtmosphereRenderPlan AtmospherePlan = {};
	TEN::Scripting::AtmosphereCelestialRenderData AtmosphereCelestialData = {};

	bool HasLegacySky() const;
	bool HasAtmosphere() const;
	bool HasAnySkyOrAtmosphere() const;
};

inline bool SkyAtmosphereRenderData::HasLegacySky() const
{
	return LegacySkyEnabled;
}

inline bool SkyAtmosphereRenderData::HasAtmosphere() const
{
	return AtmospherePlan.HasAnyPass() || AtmosphereCelestialData.HasAnyPass();
}

inline bool SkyAtmosphereRenderData::HasAnySkyOrAtmosphere() const
{
	return HasLegacySky() || HasAtmosphere();
}

struct Level : public ScriptInterfaceLevel
{
	Fog			Fog			 = {};
	int			LevelFarView = 0;
	std::string AmbientTrack = {};

	SkyLayer Layer1 = {};
	SkyLayer Layer2 = {};
	TEN::Scripting::Horizon Horizon1 = {};
	TEN::Scripting::Horizon Horizon2 = {};
	TEN::Scripting::LensFlare LensFlare = {};
	TEN::Scripting::Starfield Starfield = {};
	TEN::Scripting::Atmosphere Atmosphere = {};
	TEN::Scripting::AtmosphereEnvironmentProfile AtmosphereEnvironment = {};
	TEN::Scripting::AtmosphereCelestialProfile AtmosphereCelestial = {};

	WeatherType Weather				= WeatherType::None;
	float		WeatherStrength		= 1.0f;
	bool		WeatherClustering	= true;
	bool		Storm				= false;
	bool		Rumble				= false;

	LaraType Type = LaraType::Normal;
	int LevelSecrets = 0;
	std::vector<InventoryItem> InventoryObjects = {};

	bool ResetHub = false;

	// TODO: Clean up this mess.

	RGBAColor8Byte GetFogColor() const override;
	float GetWeatherStrength() const override;
	bool GetSkyLayerEnabled(int index) const override;
	bool GetStormEnabled() const override;
	bool GetRumbleEnabled() const override;
	short GetSkyLayerSpeed(int index) const override;
	RGBAColor8Byte GetSkyLayerColor(int index) const override;
	LaraType GetLaraType() const override;
	void SetWeatherStrength(float val);
	static void Register(sol::table& parent);
	WeatherType GetWeatherType() const override;
	bool GetWeatherClustering() const override;
	float GetFogMinDistance() const override;
	float GetFogMaxDistance() const override;
	float GetFarView() const override;
	void SetSecrets(int secrets);
	int GetSecrets() const override;
	std::string GetAmbientTrack() const override;
	bool GetResetHubEnabled() const override;

	// Atmosphere getters
	const TEN::Scripting::Atmosphere& GetAtmosphere() const;
	const TEN::Scripting::AtmosphereEnvironmentProfile& GetAtmosphereEnvironment() const;
	const TEN::Scripting::AtmosphereCelestialProfile& GetAtmosphereCelestial() const;
	TEN::Scripting::AtmosphereRuntimeSnapshot CreateAtmosphereRuntimeSnapshot() const;
	TEN::Scripting::AtmosphereRenderData CreateAtmosphereRenderData() const;
	TEN::Scripting::AtmosphereRenderPlan CreateAtmosphereRenderPlan() const;
	TEN::Scripting::AtmosphereCelestialRenderData CreateAtmosphereCelestialRenderData() const;
	TEN::Scripting::AtmosphereEnvironmentRuntimeData CreateAtmosphereEnvironmentRuntimeData() const;
	TEN::Scripting::AtmosphereRuntimeState CreateAtmosphereRuntimeState() const;
	SkyAtmosphereRenderData CreateSkyAtmosphereRenderData() const;
	bool GetAtmosphereEnabled() const;
	bool GetAtmosphereEnvironmentEnabled() const;
	bool GetAtmosphereCelestialEnabled() const;
	int GetAtmosphereCelestialBodyCount() const;
	bool GetAtmosphereCelestialHasSpaceBodies() const;

	// Horizon getters
	bool GetHorizonEnabled(int index) const override;
	GAME_OBJECT_ID GetHorizonObjectID(int index) const override;
	float GetHorizonTransparency(int index) const override;
	Vector3 GetHorizonPosition(int index) const override;
	EulerAngles GetHorizonOrientation(int index) const override;
	Vector3 GetHorizonPrevPosition(int index) const override;
	EulerAngles GetHorizonPrevOrientation(int index) const override;

	// Compatibility
	bool GetHorizon1Enabled() const;
	void SetHorizon1Enabled(bool enabled);

	// Lens flare getters
	bool  GetLensFlareEnabled() const override;
	int	  GetLensFlareSunSpriteID() const override;
	short GetLensFlarePitch() const override;
	short GetLensFlareYaw() const override;
	Color GetLensFlareColor() const override;

	// Starfield getters
	int	  GetStarfieldStarCount() const override;
	int	  GetStarfieldMeteorCount() const override;
	int	  GetStarfieldMeteorSpawnDensity() const override;
	float GetStarfieldMeteorVelocity() const override;
};

inline TEN::Scripting::AtmosphereEnvironmentRuntimeData Level::CreateAtmosphereEnvironmentRuntimeData() const
{
	return TEN::Scripting::CreateAtmosphereEnvironmentRuntimeData(AtmosphereEnvironment);
}

inline TEN::Scripting::AtmosphereRuntimeState Level::CreateAtmosphereRuntimeState() const
{
	TEN::Scripting::AtmosphereRuntimeState state = {};
	state.Snapshot = CreateAtmosphereRuntimeSnapshot();
	state.RenderData = CreateAtmosphereRenderData();
	state.RenderPlan = CreateAtmosphereRenderPlan();
	state.Environment = CreateAtmosphereEnvironmentRuntimeData();
	state.Celestial = CreateAtmosphereCelestialRenderData();
	return state;
}
