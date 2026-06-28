#pragma once

#include "Game/Gui.h"
#include "Game/LightingSettings.h"
#include "Game/LightingSettingsAdjust.h"
#include "Sound/sound.h"
#include "Specific/Input/Input.h"
#include "Specific/level.h"

namespace TEN::Gui
{
	inline void RestoreLightingSettings(GameConfiguration& settings)
	{
		settings.EnableHDRRendering = g_Configuration.EnableHDRRendering;
		settings.HDRExposure = g_Configuration.HDRExposure;
		settings.HDRStrength = g_Configuration.HDRStrength;
		settings.EnableLightBloom = g_Configuration.EnableLightBloom;
		settings.BloomStrength = g_Configuration.BloomStrength;
		settings.BloomThreshold = g_Configuration.BloomThreshold;
		settings.GlareStrength = g_Configuration.GlareStrength;
		settings.GlareLength = g_Configuration.GlareLength;
	}

	inline void ApplyLightingSettings(const GameConfiguration& settings, bool hdrRenderTargetsEnabled, bool& restartRequired)
	{
		restartRequired = settings.EnableHDRRendering != hdrRenderTargetsEnabled;
		g_Configuration.EnableHDRRendering = settings.EnableHDRRendering;
		g_Configuration.HDRExposure = settings.HDRExposure;
		g_Configuration.HDRStrength = settings.HDRStrength;
		g_Configuration.EnableLightBloom = settings.EnableLightBloom;
		g_Configuration.BloomStrength = settings.BloomStrength;
		g_Configuration.BloomThreshold = settings.BloomThreshold;
		g_Configuration.GlareStrength = settings.GlareStrength;
		g_Configuration.GlareLength = settings.GlareLength;
		g_Configuration.SaveLightingConfiguration();
	}

	inline bool UpdateLightingSettingsInput(bool& restartRequired, bool hdrRenderTargetsEnabled)
	{
		if (g_Gui.GetMenuToDisplay() == Menu::Display &&
			g_Gui.GetSelectedOption() == 7 &&
			(IsClicked(In::Select) || IsClicked(In::Action)))
		{
			g_Gui.SetMenuToDisplay(Menu::LightingHDR);
			g_Gui.SetSelectedOption(0);
			restartRequired = g_Gui.GetCurrentSettings().Configuration.EnableHDRRendering != hdrRenderTargetsEnabled;
			SoundEffect(SFX_TR4_MENU_SELECT, nullptr, SoundEnvironment::Always);
			return true;
		}

		if (g_Gui.GetMenuToDisplay() != Menu::LightingHDR)
			return false;

		auto& settings = g_Gui.GetCurrentSettings().Configuration;
		int selected = g_Gui.GetSelectedOption();
		bool moved = false;
		if (IsPulsed(In::Forward, 0.08f, 0.35f))
		{
			selected--;
			moved = true;
		}
		else if (IsPulsed(In::Back, 0.08f, 0.35f))
		{
			selected++;
			moved = true;
		}

		selected = g_Gui.GetLoopedSelectedOption(
			selected,
			(int)LightingSettingsOption::Count - 1,
			g_Configuration.MenuOptionLoopingMode == MenuOptionLoopingMode::AllMenus);
		g_Gui.SetSelectedOption(selected);

		if (moved)
			SoundEffect(SFX_TR4_MENU_CHOOSE, nullptr, SoundEnvironment::Always);

		int direction = 0;
		if (IsPulsed(In::Left, 0.08f, 0.35f))
			direction = -1;
		else if (IsPulsed(In::Right, 0.08f, 0.35f))
			direction = 1;

		if (direction != 0 && selected < (int)LightingSettingsOption::Apply)
		{
			AdjustLightingSetting(settings, (LightingSettingsOption)selected, direction);
			restartRequired = settings.EnableHDRRendering != hdrRenderTargetsEnabled;
			SoundEffect(SFX_TR4_MENU_CHOOSE, nullptr, SoundEnvironment::Always);
		}

		bool select = IsClicked(In::Select) || IsClicked(In::Action);
		bool cancel = IsClicked(In::Deselect) || IsClicked(In::Draw);
		if (cancel || (select && (LightingSettingsOption)selected == LightingSettingsOption::Cancel))
		{
			RestoreLightingSettings(settings);
			restartRequired = g_Configuration.EnableHDRRendering != hdrRenderTargetsEnabled;
			g_Gui.SetMenuToDisplay(Menu::Display);
			g_Gui.SetSelectedOption(7);
			SoundEffect(SFX_TR4_MENU_SELECT, nullptr, SoundEnvironment::Always);
			return false;
		}

		if (select && (LightingSettingsOption)selected == LightingSettingsOption::Apply)
		{
			ApplyLightingSettings(settings, hdrRenderTargetsEnabled, restartRequired);
			SoundEffect(SFX_TR4_MENU_SELECT, nullptr, SoundEnvironment::Always);
		}

		return true;
	}
}
