#pragma once
#include "Math/Math.h"
#include "Specific/Input/Input.h"
#include "Renderer/RendererEnums.h"

using namespace TEN::Input;
using namespace TEN::Math;

constexpr auto REGKEY_ROOT = "Software\\TombEngine\\1.7.0";
constexpr auto REGKEY_GRAPHICS = "Graphics";
constexpr auto REGKEY_SOUND = "Sound";
constexpr auto REGKEY_GAMEPLAY = "Gameplay";
constexpr auto REGKEY_INPUT = "Input";

constexpr auto REGKEY_SCREEN_WIDTH = "ScreenWidth";
constexpr auto REGKEY_SCREEN_HEIGHT = "ScreenHeight";
constexpr auto REGKEY_ENABLE_WINDOWED_MODE = "EnableWindowedMode";
constexpr auto REGKEY_SHADOWS = "ShadowsMode";
constexpr auto REGKEY_SHADOW_MAP_SIZE = "ShadowMapSize";
constexpr auto REGKEY_SHADOW_BLOBS_MAX = "ShadowBlobsMax";
constexpr auto REGKEY_ENABLE_CAUSTICS = "EnableCaustics";
constexpr auto REGKEY_ENABLE_DECALS = "EnableDecals";
constexpr auto REGKEY_ANTIALIASING_MODE = "AntialiasingMode";
constexpr auto REGKEY_AMBIENT_OCCLUSION = "AmbientOcclusion";
constexpr auto REGKEY_HIGH_FRAMERATE = "EnableHighFramerate";
constexpr auto REGKEY_ENABLE_HDR_RENDERING = "EnableHDRRendering";
constexpr auto REGKEY_HDR_EXPOSURE = "HDRExposure";
constexpr auto REGKEY_HDR_STRENGTH = "HDRStrength";
constexpr auto REGKEY_ENABLE_LIGHT_BLOOM = "EnableLightBloom";
constexpr auto REGKEY_BLOOM_STRENGTH = "BloomStrength";
constexpr auto REGKEY_BLOOM_THRESHOLD = "BloomThreshold";
constexpr auto REGKEY_GLARE_STRENGTH = "GlareStrength";
constexpr auto REGKEY_GLARE_LENGTH = "GlareLength";

constexpr auto REGKEY_SOUND_DEVICE = "SoundDevice";
constexpr auto REGKEY_ENABLE_SOUND = "EnableSound";
constexpr auto REGKEY_ENABLE_REVERB = "EnableReverb";
constexpr auto REGKEY_MUSIC_VOLUME = "MusicVolume";
constexpr auto REGKEY_SFX_VOLUME = "SfxVolume";

constexpr auto REGKEY_ENABLE_SUBTITLES = "EnableSubtitles";
constexpr auto REGKEY_ENABLE_AUTO_MONKEY_JUMP = "EnableAutoMonkeySwingJump";
constexpr auto REGKEY_ENABLE_AUTO_TARGETING = "EnableAutoTargeting";
constexpr auto REGKEY_ENABLE_TARGET_HIGHLIGHTER = "EnableTargetHighlighter";
constexpr auto REGKEY_ENABLE_INTERACTION_HIGHLIGHTER = "EnableInteractionHighlighter";
constexpr auto REGKEY_ENABLE_RUMBLE = "EnableRumble";
constexpr auto REGKEY_ENABLE_THUMBSTICK_CAMERA = "EnableThumbstickCamera";

constexpr auto REGKEY_MOUSE_SENSITIVITY = "MouseSensitivity";
constexpr auto REGKEY_MENU_OPTION_LOOPING_MODE = "MenuOptionLoopingMode";

LONG GetDWORDRegKey(HKEY hKey, LPCSTR strValueName, DWORD* nValue, DWORD nDefaultValue);
LONG GetBoolRegKey(HKEY hKey, LPCSTR strValueName, bool* bValue, bool bDefaultValue);
LONG GetStringRegKey(HKEY hKey, LPCSTR strValueName, char** strValue, char* strDefaultValue);
LONG SetDWORDRegKey(HKEY hKey, LPCSTR strValueName, DWORD nValue);
LONG SetBoolRegKey(HKEY hKey, LPCSTR strValueName, bool bValue);
LONG SetStringRegKey(HKEY hKey, LPCSTR strValueName, char* strValue);

enum class MenuOptionLoopingMode
{
	AllMenus,
	SaveLoadOnly,
	Disabled
};

struct GameConfiguration
{
	static constexpr auto DEFAULT_SHADOW_MAP_SIZE = 1024;
	static constexpr auto DEFAULT_SHADOW_BLOBS_MAX = 16;
	static constexpr auto DEFAULT_MOUSE_SENSITIVITY = 6;
	static constexpr auto DEFAULT_HDR_EXPOSURE = 100;
	static constexpr auto DEFAULT_HDR_STRENGTH = 100;
	static constexpr auto DEFAULT_BLOOM_STRENGTH = 100;
	static constexpr auto DEFAULT_BLOOM_THRESHOLD = 100;
	static constexpr auto DEFAULT_GLARE_STRENGTH = 35;
	static constexpr auto DEFAULT_GLARE_LENGTH = 100;

	int ScreenWidth = 0;
	int ScreenHeight = 0;
	bool EnableWindowedMode = false;
	ShadowMode ShadowType = ShadowMode::None;
	int ShadowMapSize = DEFAULT_SHADOW_MAP_SIZE;
	int ShadowBlobsMax = DEFAULT_SHADOW_BLOBS_MAX;
	bool EnableCaustics = false;
	bool EnableDecals = true;
	bool EnableAmbientOcclusion = false;
	bool EnableHighFramerate = true;
	AntialiasingMode AntialiasingMode = AntialiasingMode::None;
	bool EnableHDRRendering = false;
	int HDRExposure = DEFAULT_HDR_EXPOSURE;
	int HDRStrength = DEFAULT_HDR_STRENGTH;
	bool EnableLightBloom = false;
	int BloomStrength = DEFAULT_BLOOM_STRENGTH;
	int BloomThreshold = DEFAULT_BLOOM_THRESHOLD;
	int GlareStrength = DEFAULT_GLARE_STRENGTH;
	int GlareLength = DEFAULT_GLARE_LENGTH;

	int SoundDevice = 0;
	bool EnableSound = false;
	bool EnableReverb = false;
	int MusicVolume = 0;
	int SfxVolume = 0;

	bool EnableSubtitles = false;
	bool EnableAutoMonkeySwingJump = false;
	bool EnableAutoTargeting = false;
	bool EnableTargetHighlighter = false;
	bool EnableInteractionHighlighter = false;
	bool EnableRumble = false;
	bool EnableThumbstickCamera = false;

	int MouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;
	MenuOptionLoopingMode MenuOptionLoopingMode = MenuOptionLoopingMode::SaveLoadOnly;
	BindingProfile Bindings = {};

	std::vector<Vector2i> SupportedScreenResolutions = {};
	std::string AdapterName = {};

	GameConfiguration()
	{
		LoadLightingConfiguration();
	}

	void LoadLightingConfiguration()
	{
		HKEY rootKey = NULL;
		HKEY graphicsKey = NULL;
		if (RegOpenKeyExA(HKEY_CURRENT_USER, REGKEY_ROOT, 0, KEY_READ, &rootKey) != ERROR_SUCCESS)
			return;

		if (RegOpenKeyExA(rootKey, REGKEY_GRAPHICS, 0, KEY_READ, &graphicsKey) != ERROR_SUCCESS)
		{
			RegCloseKey(rootKey);
			return;
		}

		DWORD value = 0;
		GetBoolRegKey(graphicsKey, REGKEY_ENABLE_HDR_RENDERING, &EnableHDRRendering, false);
		GetDWORDRegKey(graphicsKey, REGKEY_HDR_EXPOSURE, &value, DEFAULT_HDR_EXPOSURE);
		HDRExposure = std::clamp((int)value, 25, 400);
		GetDWORDRegKey(graphicsKey, REGKEY_HDR_STRENGTH, &value, DEFAULT_HDR_STRENGTH);
		HDRStrength = std::clamp((int)value, 0, 100);
		GetBoolRegKey(graphicsKey, REGKEY_ENABLE_LIGHT_BLOOM, &EnableLightBloom, false);
		GetDWORDRegKey(graphicsKey, REGKEY_BLOOM_STRENGTH, &value, DEFAULT_BLOOM_STRENGTH);
		BloomStrength = std::clamp((int)value, 0, 300);
		GetDWORDRegKey(graphicsKey, REGKEY_BLOOM_THRESHOLD, &value, DEFAULT_BLOOM_THRESHOLD);
		BloomThreshold = std::clamp((int)value, 25, 300);
		GetDWORDRegKey(graphicsKey, REGKEY_GLARE_STRENGTH, &value, DEFAULT_GLARE_STRENGTH);
		GlareStrength = std::clamp((int)value, 0, 300);
		GetDWORDRegKey(graphicsKey, REGKEY_GLARE_LENGTH, &value, DEFAULT_GLARE_LENGTH);
		GlareLength = std::clamp((int)value, 25, 300);

		RegCloseKey(graphicsKey);
		RegCloseKey(rootKey);
	}

	bool SaveLightingConfiguration() const
	{
		HKEY rootKey = NULL;
		HKEY graphicsKey = NULL;
		if (RegCreateKeyExA(HKEY_CURRENT_USER, REGKEY_ROOT, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &rootKey, NULL) != ERROR_SUCCESS)
			return false;

		if (RegCreateKeyExA(rootKey, REGKEY_GRAPHICS, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &graphicsKey, NULL) != ERROR_SUCCESS)
		{
			RegCloseKey(rootKey);
			return false;
		}

		bool success =
			SetBoolRegKey(graphicsKey, REGKEY_ENABLE_HDR_RENDERING, EnableHDRRendering) == ERROR_SUCCESS &&
			SetDWORDRegKey(graphicsKey, REGKEY_HDR_EXPOSURE, HDRExposure) == ERROR_SUCCESS &&
			SetDWORDRegKey(graphicsKey, REGKEY_HDR_STRENGTH, HDRStrength) == ERROR_SUCCESS &&
			SetBoolRegKey(graphicsKey, REGKEY_ENABLE_LIGHT_BLOOM, EnableLightBloom) == ERROR_SUCCESS &&
			SetDWORDRegKey(graphicsKey, REGKEY_BLOOM_STRENGTH, BloomStrength) == ERROR_SUCCESS &&
			SetDWORDRegKey(graphicsKey, REGKEY_BLOOM_THRESHOLD, BloomThreshold) == ERROR_SUCCESS &&
			SetDWORDRegKey(graphicsKey, REGKEY_GLARE_STRENGTH, GlareStrength) == ERROR_SUCCESS &&
			SetDWORDRegKey(graphicsKey, REGKEY_GLARE_LENGTH, GlareLength) == ERROR_SUCCESS;

		RegCloseKey(graphicsKey);
		RegCloseKey(rootKey);
		return success;
	}
};

void LoadResolutionsInCombobox(HWND handle);
void LoadSoundDevicesInCombobox(HWND handle);
BOOL CALLBACK DialogProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);
int SetupDialog();
void InitDefaultConfiguration();
bool LoadConfiguration();
bool SaveConfiguration();
void SaveAudioConfig();

extern GameConfiguration g_Configuration;