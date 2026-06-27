#pragma once

#include "Game/Gui.h"
#include "Game/LightingSettings.h"
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

		return g_Gui.GetMenuToDisplay() == Menu::LightingHDR;
	}
}