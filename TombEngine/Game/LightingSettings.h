#pragma once

#include "Game/Gui.h"
#include "Specific/Input/Input.h"
#include "Specific/configuration.h"

namespace TEN::Gui
{
	enum class LightingSettingsOption
	{
		HDRRendering,
		HDRExposure,
		HDRStrength,
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

	void UpdateLightingSettingsMenu();
}
