#pragma once

#include "Game/LightingSettings.h"
#include "Specific/configuration.h"

namespace TEN::Gui
{
	inline void AdjustLightingSetting(GameConfiguration& settings, LightingSettingsOption option, int direction)
	{
		switch (option)
		{
		case LightingSettingsOption::HDRRendering:
			settings.EnableHDRRendering = !settings.EnableHDRRendering;
			break;
		case LightingSettingsOption::HDRExposure:
			settings.HDRExposure = std::clamp(settings.HDRExposure + direction * 10, 25, 400);
			break;
		case LightingSettingsOption::HDRStrength:
			settings.HDRStrength = std::clamp(settings.HDRStrength + direction * 5, 0, 100);
			break;
		case LightingSettingsOption::LightBloom:
			settings.EnableLightBloom = !settings.EnableLightBloom;
			break;
		case LightingSettingsOption::BloomStrength:
			settings.BloomStrength = std::clamp(settings.BloomStrength + direction * 10, 0, 300);
			break;
		case LightingSettingsOption::BloomThreshold:
			settings.BloomThreshold = std::clamp(settings.BloomThreshold + direction * 10, 25, 300);
			break;
		case LightingSettingsOption::GlareStrength:
			settings.GlareStrength = std::clamp(settings.GlareStrength + direction * 10, 0, 300);
			break;
		case LightingSettingsOption::GlareLength:
			settings.GlareLength = std::clamp(settings.GlareLength + direction * 10, 25, 300);
			break;
		default:
			break;
		}
	}
}