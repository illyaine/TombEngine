#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Scripting/Include/ScriptInterfaceLevel.h"
#include "Scripting/Internal/TEN/Types/Color/Color.h"

namespace TEN::Scripting
{
	enum class AtmosphereCelestialBodyType
	{
		Planet,
		Moon,
		Star,
		Comet,
		Asteroid,
		SpaceDebris,
		Nebula,
		GalaxyBand,
		Custom
	};

	enum class AtmosphereCelestialRenderMode
	{
		Billboard,
		Sphere3D,
		HorizonObject,
		GeneratedLayer,
		Custom
	};

	/// Constants for atmosphere celestial body types.
	// @enum Flow.AtmosphereCelestialBodyType
	// @pragma nostrip
	static const std::unordered_map<std::string, AtmosphereCelestialBodyType> ATMOSPHERE_CELESTIAL_BODY_TYPES
	{
		/// Planet or large planet-like sky body.
		// @mem Planet
		{ "Planet", AtmosphereCelestialBodyType::Planet },

		/// Moon or natural satellite.
		// @mem Moon
		{ "Moon", AtmosphereCelestialBodyType::Moon },

		/// Large visible star or sun-like object.
		// @mem Star
		{ "Star", AtmosphereCelestialBodyType::Star },

		/// Comet with optional tail treatment.
		// @mem Comet
		{ "Comet", AtmosphereCelestialBodyType::Comet },

		/// Asteroid or rock body.
		// @mem Asteroid
		{ "Asteroid", AtmosphereCelestialBodyType::Asteroid },

		/// Distant orbital debris or wreckage.
		// @mem SpaceDebris
		{ "SpaceDebris", AtmosphereCelestialBodyType::SpaceDebris },

		/// Nebula or distant gas cloud layer.
		// @mem Nebula
		{ "Nebula", AtmosphereCelestialBodyType::Nebula },

		/// Milky-way style galaxy band.
		// @mem GalaxyBand
		{ "GalaxyBand", AtmosphereCelestialBodyType::GalaxyBand },

		/// Custom celestial body type.
		// @mem Custom
		{ "Custom", AtmosphereCelestialBodyType::Custom }
	};

	/// Constants for atmosphere celestial render modes.
	// @enum Flow.AtmosphereCelestialRenderMode
	// @pragma nostrip
	static const std::unordered_map<std::string, AtmosphereCelestialRenderMode> ATMOSPHERE_CELESTIAL_RENDER_MODES
	{
		/// Camera-facing textured sky body.
		// @mem Billboard
		{ "Billboard", AtmosphereCelestialRenderMode::Billboard },

		/// Three-dimensional sphere-like body.
		// @mem Sphere3D
		{ "Sphere3D", AtmosphereCelestialRenderMode::Sphere3D },

		/// Bridge to an existing horizon object.
		// @mem HorizonObject
		{ "HorizonObject", AtmosphereCelestialRenderMode::HorizonObject },

		/// Generated broad sky layer such as a galaxy band.
		// @mem GeneratedLayer
		{ "GeneratedLayer", AtmosphereCelestialRenderMode::GeneratedLayer },

		/// Custom or future renderer-specific mode.
		// @mem Custom
		{ "Custom", AtmosphereCelestialRenderMode::Custom }
	};

	struct AtmosphereCelestialBodyProfile
	{
		bool Enabled{ false };
		AtmosphereCelestialBodyType Type{ AtmosphereCelestialBodyType::Planet };
		AtmosphereCelestialRenderMode RenderMode{ AtmosphereCelestialRenderMode::Sphere3D };
		std::string Name = {};
		std::string TextureName = {};
		std::string HorizonName = {};
		float Pitch{ -18.0f };
		float Yaw{ 225.0f };
		float Distance{ 1.0f };
		float Size{ 1.0f };
		float Intensity{ 1.0f };
		float HaloIntensity{ 0.0f };
		float RotationSpeed{ 0.0f };
		float OrbitSpeed{ 0.0f };
		float Parallax{ 0.0f };
		float TailLength{ 0.0f };
		int Layer{ 0 };
		bool FadeWithFog{ true };
		bool OccludesStars{ false };
		bool DrivesLight{ false };
		bool VisualOnly{ true };

		byte ColorR{ 255 };
		byte ColorG{ 255 };
		byte ColorB{ 255 };
		byte LightColorR{ 255 };
		byte LightColorG{ 255 };
		byte LightColorB{ 255 };

		void SetColor(Types::ScriptColor const& color);
		void SetLightColor(Types::ScriptColor const& color);
		Types::ScriptColor GetColor() const;
		Types::ScriptColor GetLightColor() const;
		bool IsSpaceObject() const;

		static void Register(sol::table& parent);
	};

	struct AtmosphereCelestialProfile
	{
		bool Enabled{ false };
		std::vector<AtmosphereCelestialBodyProfile> Bodies = {};
		bool HideLegacyMoonWhenActive{ false };
		bool HideStarfieldWhenSpaceLayerActive{ false };
		bool UseLayerOrdering{ true };

		int GetEnabledBodyCount() const;
		int GetEnabledSpaceBodyCount() const;
		bool HasEnabledBodies() const;
		bool HasSpaceBodies() const;

		static void Register(sol::table& parent);
	};

	struct AtmosphereCelestialRenderData
	{
		bool Enabled{ false };
		std::vector<AtmosphereCelestialBodyProfile> Bodies = {};
		int BodyCount{ 0 };
		int SpaceBodyCount{ 0 };
		int Sphere3DBodyCount{ 0 };
		int BillboardBodyCount{ 0 };
		int HorizonObjectBodyCount{ 0 };
		int GeneratedLayerCount{ 0 };
		int LightSourceCount{ 0 };
		bool HideLegacyMoon{ false };
		bool HideStarfield{ false };

		bool HasAnyPass() const;
		bool HasSkyObjects() const;
		bool Has3DBodies() const;
		bool HasBillboards() const;
		bool HasHorizonObjects() const;
		bool HasGeneratedLayers() const;
		bool HasLightSources() const;
	};

	inline void AtmosphereCelestialBodyProfile::SetColor(Types::ScriptColor const& color)
	{
		ColorR = color.GetR();
		ColorG = color.GetG();
		ColorB = color.GetB();
	}

	inline void AtmosphereCelestialBodyProfile::SetLightColor(Types::ScriptColor const& color)
	{
		LightColorR = color.GetR();
		LightColorG = color.GetG();
		LightColorB = color.GetB();
	}

	inline Types::ScriptColor AtmosphereCelestialBodyProfile::GetColor() const
	{
		return Types::ScriptColor{ ColorR, ColorG, ColorB };
	}

	inline Types::ScriptColor AtmosphereCelestialBodyProfile::GetLightColor() const
	{
		return Types::ScriptColor{ LightColorR, LightColorG, LightColorB };
	}

	inline bool AtmosphereCelestialBodyProfile::IsSpaceObject() const
	{
		return Type == AtmosphereCelestialBodyType::Planet ||
			Type == AtmosphereCelestialBodyType::Moon ||
			Type == AtmosphereCelestialBodyType::Star ||
			Type == AtmosphereCelestialBodyType::Comet ||
			Type == AtmosphereCelestialBodyType::Asteroid ||
			Type == AtmosphereCelestialBodyType::SpaceDebris;
	}

	inline int AtmosphereCelestialProfile::GetEnabledBodyCount() const
	{
		int count = 0;

		for (auto const& body : Bodies)
		{
			if (body.Enabled)
				count++;
		}

		return count;
	}

	inline int AtmosphereCelestialProfile::GetEnabledSpaceBodyCount() const
	{
		int count = 0;

		for (auto const& body : Bodies)
		{
			if (body.Enabled && body.IsSpaceObject())
				count++;
		}

		return count;
	}

	inline bool AtmosphereCelestialProfile::HasEnabledBodies() const
	{
		return GetEnabledBodyCount() > 0;
	}

	inline bool AtmosphereCelestialProfile::HasSpaceBodies() const
	{
		return GetEnabledSpaceBodyCount() > 0;
	}

	inline bool AtmosphereCelestialRenderData::HasAnyPass() const
	{
		return Enabled && BodyCount > 0;
	}

	inline bool AtmosphereCelestialRenderData::HasSkyObjects() const
	{
		return Has3DBodies() || HasBillboards() || HasGeneratedLayers() || HasHorizonObjects();
	}

	inline bool AtmosphereCelestialRenderData::Has3DBodies() const
	{
		return Sphere3DBodyCount > 0;
	}

	inline bool AtmosphereCelestialRenderData::HasBillboards() const
	{
		return BillboardBodyCount > 0;
	}

	inline bool AtmosphereCelestialRenderData::HasHorizonObjects() const
	{
		return HorizonObjectBodyCount > 0;
	}

	inline bool AtmosphereCelestialRenderData::HasGeneratedLayers() const
	{
		return GeneratedLayerCount > 0;
	}

	inline bool AtmosphereCelestialRenderData::HasLightSources() const
	{
		return LightSourceCount > 0;
	}

	inline AtmosphereCelestialRenderData CreateAtmosphereCelestialRenderData(AtmosphereCelestialProfile const& profile)
	{
		AtmosphereCelestialRenderData data = {};
		data.Enabled = profile.Enabled;

		if (!profile.Enabled)
			return data;

		data.Bodies.reserve(profile.Bodies.size());

		for (auto const& body : profile.Bodies)
		{
			if (!body.Enabled)
				continue;

			data.Bodies.push_back(body);
			data.BodyCount++;

			if (body.IsSpaceObject())
				data.SpaceBodyCount++;

			if (body.DrivesLight)
				data.LightSourceCount++;

			switch (body.RenderMode)
			{
			case AtmosphereCelestialRenderMode::Sphere3D:
				data.Sphere3DBodyCount++;
				break;

			case AtmosphereCelestialRenderMode::Billboard:
				data.BillboardBodyCount++;
				break;

			case AtmosphereCelestialRenderMode::HorizonObject:
				data.HorizonObjectBodyCount++;
				break;

			case AtmosphereCelestialRenderMode::GeneratedLayer:
				data.GeneratedLayerCount++;
				break;

			case AtmosphereCelestialRenderMode::Custom:
				data.GeneratedLayerCount++;
				break;
			}
		}

		data.HideLegacyMoon = profile.HideLegacyMoonWhenActive && data.BodyCount > 0;
		data.HideStarfield = profile.HideStarfieldWhenSpaceLayerActive && data.SpaceBodyCount > 0;
		return data;
	}

	/*** Celestial sky object data, such as planets, moons, comets, galaxy bands, or space debris.
	@tenprimitive Flow.AtmosphereCelestialBodyProfile
	@pragma nostrip
	*/
	inline void AtmosphereCelestialBodyProfile::Register(sol::table& parent)
	{
		using ctors = sol::constructors<AtmosphereCelestialBodyProfile()>;
		parent.new_usertype<AtmosphereCelestialBodyProfile>("AtmosphereCelestialBodyProfile",
			ctors(),
			sol::call_constructor, ctors(),

			/// (bool) Enable this celestial body.
			//@mem enabled
			"enabled", &AtmosphereCelestialBodyProfile::Enabled,

			/// (AtmosphereCelestialBodyType) Select the body type.
			//@mem type
			"type", &AtmosphereCelestialBodyProfile::Type,

			/// (AtmosphereCelestialRenderMode) Select how this body should be rendered later.
			//@mem renderMode
			"renderMode", &AtmosphereCelestialBodyProfile::RenderMode,

			/// (string) Optional builder-facing body name.
			//@mem name
			"name", &AtmosphereCelestialBodyProfile::Name,

			/// (string) Optional texture name.
			//@mem textureName
			"textureName", &AtmosphereCelestialBodyProfile::TextureName,

			/// (string) Optional horizon object bridge name.
			//@mem horizonName
			"horizonName", &AtmosphereCelestialBodyProfile::HorizonName,

			/// (float) Sky pitch angle in degrees.
			//@mem pitch
			"pitch", &AtmosphereCelestialBodyProfile::Pitch,

			/// (float) Sky yaw angle in degrees.
			//@mem yaw
			"yaw", &AtmosphereCelestialBodyProfile::Yaw,

			/// (float) Relative sky distance.
			//@mem distance
			"distance", &AtmosphereCelestialBodyProfile::Distance,

			/// (float) Relative visible size.
			//@mem size
			"size", &AtmosphereCelestialBodyProfile::Size,

			/// (float) Visible brightness.
			//@mem intensity
			"intensity", &AtmosphereCelestialBodyProfile::Intensity,

			/// (float) Halo brightness.
			//@mem haloIntensity
			"haloIntensity", &AtmosphereCelestialBodyProfile::HaloIntensity,

			/// (float) Self rotation speed.
			//@mem rotationSpeed
			"rotationSpeed", &AtmosphereCelestialBodyProfile::RotationSpeed,

			/// (float) Orbital movement speed.
			//@mem orbitSpeed
			"orbitSpeed", &AtmosphereCelestialBodyProfile::OrbitSpeed,

			/// (float) Parallax amount.
			//@mem parallax
			"parallax", &AtmosphereCelestialBodyProfile::Parallax,

			/// (float) Optional comet or debris trail length.
			//@mem tailLength
			"tailLength", &AtmosphereCelestialBodyProfile::TailLength,

			/// (int) Layer order inside the celestial stack.
			//@mem layer
			"layer", &AtmosphereCelestialBodyProfile::Layer,

			/// (bool) Fade this body with level fog.
			//@mem fadeWithFog
			"fadeWithFog", &AtmosphereCelestialBodyProfile::FadeWithFog,

			/// (bool) Allow this body to occlude stars later.
			//@mem occludesStars
			"occludesStars", &AtmosphereCelestialBodyProfile::OccludesStars,

			/// (bool) Allow this body to act as a future light source.
			//@mem drivesLight
			"drivesLight", &AtmosphereCelestialBodyProfile::DrivesLight,

			/// (bool) Keep this body visual-only.
			//@mem visualOnly
			"visualOnly", &AtmosphereCelestialBodyProfile::VisualOnly,

			/// (@{Color}) Visible body color.
			//@mem color
			"color", sol::property(&AtmosphereCelestialBodyProfile::GetColor, &AtmosphereCelestialBodyProfile::SetColor),

			/// (@{Color}) Optional light color.
			//@mem lightColor
			"lightColor", sol::property(&AtmosphereCelestialBodyProfile::GetLightColor, &AtmosphereCelestialBodyProfile::SetLightColor),

			/// (function) Returns whether this body is treated as a concrete space object.
			//@mem isSpaceObject
			"isSpaceObject", &AtmosphereCelestialBodyProfile::IsSpaceObject
		);
	}

	/*** Celestial sky stack settings. To be used with @{Flow.Level.atmosphereCelestial}.
	@tenprimitive Flow.AtmosphereCelestialProfile
	@pragma nostrip
	*/
	inline void AtmosphereCelestialProfile::Register(sol::table& parent)
	{
		parent.set("AtmosphereCelestialBodyType", ATMOSPHERE_CELESTIAL_BODY_TYPES);
		parent.set("AtmosphereCelestialRenderMode", ATMOSPHERE_CELESTIAL_RENDER_MODES);

		AtmosphereCelestialBodyProfile::Register(parent);

		using ctors = sol::constructors<AtmosphereCelestialProfile()>;
		parent.new_usertype<AtmosphereCelestialProfile>("AtmosphereCelestialProfile",
			ctors(),
			sol::call_constructor, ctors(),

			/// (bool) Enable celestial sky stack processing.
			//@mem enabled
			"enabled", &AtmosphereCelestialProfile::Enabled,

			/// (@{Flow.AtmosphereCelestialBodyProfile}[]) Celestial bodies and generated sky layers.
			//@mem bodies
			"bodies", &AtmosphereCelestialProfile::Bodies,

			/// (bool) Hide the legacy single moon profile when this stack has enabled bodies.
			//@mem hideLegacyMoonWhenActive
			"hideLegacyMoonWhenActive", &AtmosphereCelestialProfile::HideLegacyMoonWhenActive,

			/// (bool) Hide legacy starfield when a space layer is active.
			//@mem hideStarfieldWhenSpaceLayerActive
			"hideStarfieldWhenSpaceLayerActive", &AtmosphereCelestialProfile::HideStarfieldWhenSpaceLayerActive,

			/// (bool) Use the body layer value as later render order hint.
			//@mem useLayerOrdering
			"useLayerOrdering", &AtmosphereCelestialProfile::UseLayerOrdering,

			/// (function) Returns the number of enabled celestial bodies.
			//@mem getEnabledBodyCount
			"getEnabledBodyCount", &AtmosphereCelestialProfile::GetEnabledBodyCount,

			/// (function) Returns the number of enabled concrete space bodies.
			//@mem getEnabledSpaceBodyCount
			"getEnabledSpaceBodyCount", &AtmosphereCelestialProfile::GetEnabledSpaceBodyCount,

			/// (function) Returns true when this stack has at least one enabled body.
			//@mem hasEnabledBodies
			"hasEnabledBodies", &AtmosphereCelestialProfile::HasEnabledBodies,

			/// (function) Returns true when this stack has at least one enabled concrete space body.
			//@mem hasSpaceBodies
			"hasSpaceBodies", &AtmosphereCelestialProfile::HasSpaceBodies
		);
	}
}
