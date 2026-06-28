#pragma once

#include "Game/LightingSettings.h"
#include "Renderer/Renderer.h"
#include "Scripting/Include/Flow/ScriptInterfaceFlowHandler.h"

namespace TEN::Gui
{
	inline std::string LightingPercent(int value)
	{
		return std::to_string(value) + "%";
	}

	inline void AddLightingSettingRow(
		TEN::Renderer::Renderer& renderer,
		int y,
		int option,
		const char* label,
		const std::string& value)
	{
		bool selected = g_Gui.GetSelectedOption() == option;
		int flags = (int)PrintStringFlags::Outline |
			(selected ? (int)PrintStringFlags::Blink : 0);

		renderer.AddString(170, y, g_GameFlow->GetString(label),
			g_GameFlow->GetSettings()->UI.HeaderTextColor, flags);
		renderer.AddString(530, y, value,
			g_GameFlow->GetSettings()->UI.PlainTextColor, flags);
	}

	inline void RenderLightingSettings(TEN::Renderer::Renderer& renderer, bool restartRequired)
	{
		constexpr int center = 400;
		constexpr int centerFlags = (int)PrintStringFlags::Outline | (int)PrintStringFlags::Center;
		constexpr int spacing = 28;
		int y = 60;

		auto& settings = g_Gui.GetCurrentSettings().Configuration;
		renderer.AddString(center, y, g_GameFlow->GetString(STRING_LIGHTING_HDR),
			g_GameFlow->GetSettings()->UI.OptionTextColor, centerFlags);
		y = 115;

		AddLightingSettingRow(renderer, y, 0, STRING_HDR_RENDERING,
			g_GameFlow->GetString(settings.EnableHDRRendering ? STRING_ENABLED : STRING_DISABLED));
		y += spacing;
		AddLightingSettingRow(renderer, y, 1, STRING_HDR_EXPOSURE, LightingPercent(settings.HDRExposure));
		y += spacing;
		AddLightingSettingRow(renderer, y, 2, STRING_HDR_STRENGTH, LightingPercent(settings.HDRStrength));
		y += spacing;
		AddLightingSettingRow(renderer, y, 3, STRING_LIGHT_BLOOM,
			g_GameFlow->GetString(settings.EnableLightBloom ? STRING_ENABLED : STRING_DISABLED));
		y += spacing;
		AddLightingSettingRow(renderer, y, 4, STRING_BLOOM_STRENGTH, LightingPercent(settings.BloomStrength));
		y += spacing;
		AddLightingSettingRow(renderer, y, 5, STRING_BLOOM_THRESHOLD, LightingPercent(settings.BloomThreshold));
		y += spacing;
		AddLightingSettingRow(renderer, y, 6, STRING_BLOOM_RADIUS, LightingPercent(settings.BloomRadius));
		y += spacing;
		AddLightingSettingRow(renderer, y, 7, STRING_GLARE_STRENGTH, LightingPercent(settings.GlareStrength));
		y += spacing;
		AddLightingSettingRow(renderer, y, 8, STRING_GLARE_LENGTH, LightingPercent(settings.GlareLength));
		y += 36;

		int applyFlags = centerFlags |
			(g_Gui.GetSelectedOption() == 9 ? (int)PrintStringFlags::Blink : 0);
		renderer.AddString(center, y, g_GameFlow->GetString(STRING_APPLY),
			g_GameFlow->GetSettings()->UI.HeaderTextColor, applyFlags);
		y += spacing;

		int cancelFlags = centerFlags |
			(g_Gui.GetSelectedOption() == 10 ? (int)PrintStringFlags::Blink : 0);
		renderer.AddString(center, y, g_GameFlow->GetString(STRING_CANCEL),
			g_GameFlow->GetSettings()->UI.HeaderTextColor, cancelFlags);

		if (restartRequired)
		{
			y += 32;
			renderer.AddString(center, y, g_GameFlow->GetString(STRING_HDR_RESTART_REQUIRED),
				g_GameFlow->GetSettings()->UI.PlainTextColor, centerFlags);
		}
	}
}
