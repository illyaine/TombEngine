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

	void RenderLightingSettings(TEN::Renderer::Renderer& renderer, bool restartRequired);
}