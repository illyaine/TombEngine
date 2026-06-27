#pragma once

#include "Game/Gui.h"
#include "Game/LightingSettings.h"
#include "Game/LightingSettingsAdjust.h"
#include "Specific/Input/Input.h"

namespace TEN::Gui
{
	inline bool UpdateLightingSettingsInput(bool& restartRequired)
	{
		if (g_Gui.GetMenuToDisplay() == Menu::Display &&
			g_Gui.GetSelectedOption() == 7 &&
			(IsClicked(In::Select) || IsClicked(In::Action)))
		{
			g_Gui.SetMenuToDisplay(Menu::LightingHDR);
			g_Gui.SetSelectedOption(0);
			restartRequired = false;
			return true;
		}

		if (g_Gui.GetMenuToDisplay() != Menu::LightingHDR)
			return false;

		int selected = g_Gui.GetSelectedOption();
		if (IsPulsed(In::Forward, 0.08f, 0.35f))
			selected--;
		else if (IsPulsed(In::Back, 0.08f, 0.35f))
			selected++;

		selected = std::clamp(selected, 0, (int)LightingSettingsOption::Count - 1);
		g_Gui.SetSelectedOption(selected);

		int direction = 0;
		if (IsPulsed(In::Left, 0.08f, 0.35f))
			direction = -1;
		else if (IsPulsed(In::Right, 0.08f, 0.35f))
			direction = 1;

		if (direction != 0)
			AdjustLightingSetting(g_Gui.GetCurrentSettings().Configuration, (LightingSettingsOption)selected, direction);

		return true;
	}
}