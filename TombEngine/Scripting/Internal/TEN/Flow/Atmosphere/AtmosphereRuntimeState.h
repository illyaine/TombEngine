#pragma once

#include "Scripting/Internal/TEN/Flow/Atmosphere/Atmosphere.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereCelestialRender.h"
#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereEnvironmentRuntime.h"

namespace TEN::Scripting
{
	struct AtmosphereRuntimeState
	{
		AtmosphereRuntimeSnapshot Snapshot = {};
		AtmosphereRenderData RenderData = {};
		AtmosphereRenderPlan RenderPlan = {};
		AtmosphereEnvironmentRuntimeData Environment = {};
		AtmosphereCelestialRenderData Celestial = {};

		bool HasWeather() const;
		bool HasAurora() const;
		bool HasMoon() const;
		bool HasGeneratedEffects() const;
		bool HasLightShafts() const;
		bool HasCelestialObjects() const;
		bool HasAnySkyPass() const;
		bool HasAnyWorldPass() const;
		bool HasAnyPass() const;
		bool HasHazard() const;
		bool ShouldApplyEnvironmentDamage() const;
	};

	inline bool AtmosphereRuntimeState::HasWeather() const
	{
		return RenderData.HasWeather();
	}

	inline bool AtmosphereRuntimeState::HasAurora() const
	{
		return RenderData.HasAurora();
	}

	inline bool AtmosphereRuntimeState::HasMoon() const
	{
		return RenderData.HasMoon();
	}

	inline bool AtmosphereRuntimeState::HasGeneratedEffects() const
	{
		return RenderData.HasEffects();
	}

	inline bool AtmosphereRuntimeState::HasLightShafts() const
	{
		return RenderData.HasLightShafts();
	}

	inline bool AtmosphereRuntimeState::HasCelestialObjects() const
	{
		return Celestial.HasAnyPass();
	}

	inline bool AtmosphereRuntimeState::HasAnySkyPass() const
	{
		return RenderPlan.HasSkyPasses() || Celestial.HasSkyObjects();
	}

	inline bool AtmosphereRuntimeState::HasAnyWorldPass() const
	{
		return RenderPlan.HasWorldPasses();
	}

	inline bool AtmosphereRuntimeState::HasAnyPass() const
	{
		return RenderPlan.HasAnyPass() || Celestial.HasAnyPass();
	}

	inline bool AtmosphereRuntimeState::HasHazard() const
	{
		return Environment.HasGameplayHazard || Environment.HasHazardousPrecipitation;
	}

	inline bool AtmosphereRuntimeState::ShouldApplyEnvironmentDamage() const
	{
		return Environment.ShouldApplyDamage();
	}
}
