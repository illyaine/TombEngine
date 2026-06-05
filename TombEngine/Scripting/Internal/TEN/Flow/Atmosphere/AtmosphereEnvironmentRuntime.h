#pragma once

#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereEnvironment.h"

namespace TEN::Scripting
{
	struct AtmosphereEnvironmentRuntimeData
	{
		bool Enabled{ false };
		AtmosphereEnvironmentMode Mode{ AtmosphereEnvironmentMode::NormalOutdoor };
		AtmospherePrecipitationMaterial PrecipitationMaterial{ AtmospherePrecipitationMaterial::Water };
		bool IsSpaceMode{ false };
		bool IsAlienOrToxicMode{ false };
		bool IsArtificialEnvironment{ false };
		bool AllowsNormalPrecipitation{ true };
		bool AllowsWind{ true };
		bool HasHazardousPrecipitation{ false };
		bool HasGameplayHazard{ false };
		bool VisualOnlyHazards{ true };
		float DamagePerSecond{ 0.0f };

		bool ShouldApplyDamage() const;
		bool ShouldRenderHazardHint() const;
	};

	inline bool AtmosphereEnvironmentRuntimeData::ShouldApplyDamage() const
	{
		return Enabled && HasGameplayHazard && !VisualOnlyHazards && DamagePerSecond > 0.0f;
	}

	inline bool AtmosphereEnvironmentRuntimeData::ShouldRenderHazardHint() const
	{
		return Enabled && HasHazardousPrecipitation;
	}

	inline AtmosphereEnvironmentRuntimeData CreateAtmosphereEnvironmentRuntimeData(AtmosphereEnvironmentProfile const& profile)
	{
		AtmosphereEnvironmentRuntimeData data = {};
		data.Enabled = profile.Enabled;
		data.Mode = profile.Mode;
		data.PrecipitationMaterial = profile.PrecipitationMaterial;
		data.IsSpaceMode = profile.IsSpaceMode();
		data.IsAlienOrToxicMode = profile.IsAlienOrToxicMode();
		data.IsArtificialEnvironment = profile.IsArtificialEnvironment();
		data.AllowsNormalPrecipitation = profile.AllowsNormalPrecipitation();
		data.AllowsWind = profile.AllowsWind();
		data.HasHazardousPrecipitation = profile.HasHazardousPrecipitation();
		data.HasGameplayHazard = profile.HasGameplayHazard();
		data.VisualOnlyHazards = profile.VisualOnlyHazards;
		data.DamagePerSecond = profile.DamagePerSecond;
		return data;
	}
}
