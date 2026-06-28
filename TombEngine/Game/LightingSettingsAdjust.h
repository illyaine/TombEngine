#pragma once

#include "Game/LightingSettings.h"
#include "Specific/configuration.h"

namespace TEN::Gui
{
	struct HDRPresetValues
	{
		int Exposure;
		int ToneMapping;
		int BloomStrength;
		int BloomThreshold;
		int BloomRadius;
		int GlareStrength;
		int GlareLength;
	};

	inline HDRPresetValues GetHDRPresetValues(HDRPreset preset)
	{
		switch (preset)
		{
		case HDRPreset::Light:
			return { 105, 45, 60, 120, 80, 15, 80 };
		case HDRPreset::Strong:
			return { 145, 100, 170, 65, 140, 75, 140 };
		case HDRPreset::Medium:
		default:
			return { 120, 75, 110, 90, 110, 35, 100 };
		}
	}

	inline bool MatchesHDRPreset(const GameConfiguration& settings, HDRPreset preset)
	{
		if (preset == HDRPreset::Custom)
			return false;

		const auto values = GetHDRPresetValues(preset);
		return settings.EnableLightBloom &&
			settings.HDRExposure == values.Exposure &&
			settings.HDRStrength == values.ToneMapping &&
			settings.BloomStrength == values.BloomStrength &&
			settings.BloomThreshold == values.BloomThreshold &&
			settings.BloomRadius == values.BloomRadius &&
			settings.GlareStrength == values.GlareStrength &&
			settings.GlareLength == values.GlareLength;
	}

	inline HDRPreset DetectHDRPreset(const GameConfiguration& settings)
	{
		if (MatchesHDRPreset(settings, HDRPreset::Light))
			return HDRPreset::Light;
		if (MatchesHDRPreset(settings, HDRPreset::Medium))
			return HDRPreset::Medium;
		if (MatchesHDRPreset(settings, HDRPreset::Strong))
			return HDRPreset::Strong;

		return HDRPreset::Custom;
	}

	inline void ApplyHDRPreset(GameConfiguration& settings, HDRPreset preset)
	{
		if (preset == HDRPreset::Custom)
			return;

		const auto values = GetHDRPresetValues(preset);
		settings.EnableHDRRendering = true;
		settings.HDRExposure = values.Exposure;
		settings.HDRStrength = values.ToneMapping;
		settings.EnableLightBloom = true;
		settings.BloomStrength = values.BloomStrength;
		settings.BloomThreshold = values.BloomThreshold;
		settings.BloomRadius = values.BloomRadius;
		settings.GlareStrength = values.GlareStrength;
		settings.GlareLength = values.GlareLength;
	}

	inline void AdjustHDRPreset(GameConfiguration& settings, int direction)
	{
		auto preset = DetectHDRPreset(settings);
		int presetIndex = (preset == HDRPreset::Custom) ?
			(int)HDRPreset::Medium : (int)preset;
		presetIndex = std::clamp(presetIndex + direction, (int)HDRPreset::Light, (int)HDRPreset::Strong);
		ApplyHDRPreset(settings, (HDRPreset)presetIndex);
	}

	inline void AdjustLightingSetting(GameConfiguration& settings, LightingSettingsOption option, int direction)
	{
		switch (option)
		{
		case LightingSettingsOption::HDRRendering:
			settings.EnableHDRRendering = !settings.EnableHDRRendering;
			break;
		case LightingSettingsOption::HDRPreset:
			AdjustHDRPreset(settings, direction);
			break;
		case LightingSettingsOption::HDRExposure:
			settings.HDRExposure = std::clamp(settings.HDRExposure + direction * 10, 25, 400);
			break;
		case LightingSettingsOption::HDRStrength:
			settings.HDRStrength = std::clamp(settings.HDRStrength + direction * 5, 0, 100);
			break;
		case LightingSettingsOption::LevelBrightness:
			settings.LevelBrightness = std::clamp(settings.LevelBrightness + direction * 5, 50, 150);
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
		case LightingSettingsOption::BloomRadius:
			settings.BloomRadius = std::clamp(settings.BloomRadius + direction * 10, 25, 300);
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