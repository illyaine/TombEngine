#pragma once

#include <vector>

#include "Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereCelestial.h"

namespace TEN::Scripting
{
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
}
