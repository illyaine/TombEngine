#pragma once

#include "Game/Gui.h"
#include "Specific/Input/Input.h"
#include "Specific/configuration.h"

namespace TEN::Gui
{
	enum class HDRPreset
	{
		Light,
		Medium,
		Strong,
		Custom
	};

	enum class LightingSettingsOption
	{
		HDRRendering,
		HDRPreset,
		HDRExposure,
		HDRStrength,
		LevelBrightness,
		LightBloom,
		BloomStrength,
		BloomThreshold,
		BloomRadius,
		GlareStrength,
		GlareLength,
		Apply,
		Cancel,
		Count
	};
}