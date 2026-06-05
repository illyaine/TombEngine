# Atmosphere System Test Plan

## Purpose

This file describes what can be tested from the current `atmosphere_system` branch before visible renderer activation exists.

The current branch is not expected to show aurora, moon, light shafts, planets, galaxy layers, or generated atmosphere visuals yet. The goal of this test pass is to verify that the Flow data shape, legacy weather fallback, enum tables, environment policy, celestial activation data, and non-rendering runtime snapshot path compile and do not break existing levels.

## Expected Current Behavior

Expected visible behavior:

```text
- Existing legacy weather should still work.
- Existing levels without level.atmosphere, level.atmosphereEnvironment, or level.atmosphereCelestial should behave like before.
- level.atmosphere can be present in Flow/Lua without crashing script loading if the binding compiles.
- level.atmosphereEnvironment can be present in Flow/Lua without crashing script loading if the binding compiles.
- level.atmosphereCelestial can be present in Flow/Lua without crashing script loading if the binding compiles.
- WeatherQuality, AtmosphereEffectType, AtmosphereEffectScope, AtmosphereEffectRenderMode, AtmosphereEnvironmentMode, AtmospherePrecipitationMaterial, AtmosphereCelestialBodyType, and AtmosphereCelestialRenderMode should be available as Flow tables.
- Aurora, moon, celestial bodies, and light shaft fields should load as data only. They are not visibly rendered yet.
- Local/nullmesh effects and local/nullmesh light shafts should load as data only. They are not visibly rendered yet.
```

## Test 1: Legacy Weather Compatibility

Use an existing level script with the classic fields only:

```lua
level.weather = WeatherType.Rain
level.weatherStrength = 1.0
level.weatherClustering = true
```

Expected result:

```text
- Script loads.
- Rain still uses the legacy path.
- No atmosphere-specific visual change is expected.
```

## Test 2: Opt-In Global Atmosphere Data

Add an atmosphere profile to one level:

```lua
level.atmosphere = Flow.Atmosphere {
	enabled = true,

	weather = Flow.WeatherProfile {
		type = WeatherType.Rain,
		strength = 0.75,
		clustering = true,
		quality = WeatherQuality.Auto,

		rain = Flow.RainProfile {
			windInfluence = 0.5,
			nearDensity = 1.0,
			midDensity = 0.65,
			farDensity = 0.35,
			impacts = true,
			maxImpactsPerFrame = 32
		}
	},

	wind = Flow.WindProfile {
		direction = 45,
		strength = 0.6,
		gustStrength = 0.25,
		gustFrequency = 0.2,
		turbulence = 0.2,
		verticalDrift = 0.0
	},

	aurora = Flow.AuroraProfile {
		enabled = true,
		intensity = 0.7,
		speed = 0.25,
		height = 0.8,
		width = 1.0,
		waveScale = 1.0,
		waveStrength = 1.0,
		transparency = 1.0,
		fadeWithFog = true,
		colorA = Color(80, 180, 255),
		colorB = Color(120, 255, 180),
		colorC = Color(180, 120, 255)
	},

	moon = Flow.MoonProfile {
		enabled = true,
		pitch = -18,
		yaw = 225,
		size = 1.2,
		intensity = 0.85,
		haloIntensity = 0.35,
		lightIntensity = 0.25,
		phase = 1.0,
		fadeWithFog = true,
		drivesLightShafts = true,
		color = Color(220, 230, 255),
		lightColor = Color(180, 200, 255)
	}
}
```

Expected result:

```text
- Script loads if Flow binding is correct.
- Weather getters should use atmosphere.weather values.
- Aurora and moon are not visible yet because no renderer layer is implemented.
- Moon position is script-controlled through pitch/yaw data.
```

## Test 3: Environment Policy Data

Add an environment profile to one level:

```lua
level.atmosphereEnvironment = Flow.AtmosphereEnvironmentProfile {
	enabled = true,
	mode = Flow.AtmosphereEnvironmentMode.SpaceVacuum,
	precipitationMaterial = Flow.AtmospherePrecipitationMaterial.None,
	artificialWeather = false,
	allowToxicWeather = false,
	forceAllowPrecipitation = false,
	forceAllowWind = false,
	visualOnlyHazards = true,
	damagePerSecond = 0
}
```

Expected result:

```text
- Script loads if Flow binding is correct.
- Normal rain/snow should be blocked by the environment policy in read paths.
- Wind should be blocked in space vacuum unless artificial weather or force wind is enabled.
- No gameplay damage is active yet.
```

## Test 4: Celestial Sky Stack Data

Add a simple planet and galaxy layer:

```lua
level.atmosphereCelestial = Flow.AtmosphereCelestialProfile {
	enabled = true,
	hideLegacyMoonWhenActive = true,
	hideStarfieldWhenSpaceLayerActive = true,
	useLayerOrdering = true,

	bodies = {
		Flow.AtmosphereCelestialBodyProfile {
			enabled = true,
			type = Flow.AtmosphereCelestialBodyType.Planet,
			renderMode = Flow.AtmosphereCelestialRenderMode.Sphere3D,
			name = "earth",
			textureName = "earth_day",
			pitch = -12,
			yaw = 210,
			distance = 1.0,
			size = 1.4,
			intensity = 1.0,
			haloIntensity = 0.15,
			layer = 10,
			fadeWithFog = true,
			drivesLight = true,
			visualOnly = true,
			color = Color(255, 255, 255),
			lightColor = Color(180, 210, 255)
		},

		Flow.AtmosphereCelestialBodyProfile {
			enabled = true,
			type = Flow.AtmosphereCelestialBodyType.GalaxyBand,
			renderMode = Flow.AtmosphereCelestialRenderMode.GeneratedLayer,
			name = "milky_way",
			pitch = 8,
			yaw = 45,
			size = 2.0,
			intensity = 0.35,
			layer = 0,
			fadeWithFog = true,
			visualOnly = true,
			color = Color(180, 200, 255)
		}
	}
}
```

Expected result:

```text
- Script loads if Flow binding is correct.
- CelestialRenderData should classify one Sphere3D body and one GeneratedLayer.
- No planet or galaxy should render yet.
- No legacy moon/starfield suppression is visibly forced yet.
```

## Test 5: Generated Effect Data Shape Only

Add one global generated effect entry:

```lua
level.atmosphere.effects = {
	Flow.AtmosphereEffectProfile {
		enabled = true,
		type = AtmosphereEffectType.LeafFall,
		scope = AtmosphereEffectScope.Global,
		renderMode = AtmosphereEffectRenderMode.Generated,
		density = 0.35,
		speed = 0.25,
		direction = 35,
		turbulence = 0.3,
		minSize = 16,
		maxSize = 48,
		colorA = Color(160, 120, 40),
		colorB = Color(90, 60, 20)
	}
}
```

Expected result:

```text
- This is data-only for now.
- No leaves should render yet.
- If Lua binding fails here, defer effects table usage until Tomb Editor exports stable metadata.
```

## Test 6: Local Effect Data Shape Only

Add one local/nullmesh-style entry only after Test 5 loads:

```lua
level.atmosphere.effects = {
	Flow.AtmosphereEffectProfile {
		enabled = true,
		type = AtmosphereEffectType.GroundFog,
		scope = AtmosphereEffectScope.Nullmesh,
		anchorName = "fog_pool_01",
		renderMode = AtmosphereEffectRenderMode.Generated,
		radius = 1536,
		height = 256,
		density = 0.8,
		generatedSoftness = 1.0,
		collideWithGeometry = true,
		stopAtWalls = true,
		stopAtFloors = true,
		stopAtCeilings = true,
		clampToRoom = true
	}
}
```

Expected result:

```text
- This is data-only for now.
- No fog should render yet.
- This workflow is intended to be authored by Tomb Editor Object Parameters later, not by hand-written scripts.
```

## Test 7: Global Light Shaft Data Shape Only

Add one global moon-driven light shaft:

```lua
level.atmosphere.lightShafts = {
	Flow.LightShaftProfile {
		enabled = true,
		scope = AtmosphereEffectScope.Global,
		inheritMoonDirection = true,
		length = 4096,
		radius = 768,
		intensity = 0.45,
		density = 0.35,
		softness = 1.0,
		dustDensity = 0.15,
		fadeWithFog = true,
		blockedByGeometry = true,
		color = Color(220, 230, 255)
	}
}
```

Expected result:

```text
- This is data-only for now.
- No visible shaft should render yet.
- The shaft can inherit the moon direction later.
```

## Test 8: Local Nullmesh Light Shaft Data Shape Only

Add one local/nullmesh-style light shaft:

```lua
level.atmosphere.lightShafts = {
	Flow.LightShaftProfile {
		enabled = true,
		scope = AtmosphereEffectScope.Nullmesh,
		anchorName = "shaft_window_01",
		pitch = -45,
		yaw = 135,
		length = 3072,
		radius = 384,
		intensity = 0.6,
		density = 0.45,
		softness = 1.0,
		dustDensity = 0.25,
		blockedByGeometry = true,
		clampToRoom = true,
		color = Color(255, 235, 190)
	}
}
```

Expected result:

```text
- This is data-only for now.
- No visible shaft should render yet.
- This workflow is intended to be authored by Tomb Editor Object Parameters later.
```

## If Compile Fails

Known likely failure points:

```text
- Flow table registration for WeatherQuality / AtmosphereEffectType / AtmosphereEffectScope / AtmosphereEffectRenderMode may need to move to FlowHandler MakeReadOnlyTable style.
- Flow table registration for AtmosphereEnvironmentMode / AtmospherePrecipitationMaterial may need to move to FlowHandler MakeReadOnlyTable style.
- Flow table registration for AtmosphereCelestialBodyType / AtmosphereCelestialRenderMode may need to move to FlowHandler MakeReadOnlyTable style.
- std::vector<AtmosphereEffectProfile> or std::vector<LightShaftProfile> assignment from Lua table may need a table-wrapper pattern.
- std::vector<AtmosphereCelestialBodyProfile> assignment from Lua table may need a table-wrapper pattern.
- Inline helper compatibility may need moving some implementations from headers into .cpp files after the project file is clean.
- TombEngine.vcxproj noisy diff may need local cleanup.
```

If atmosphere enum table registration fails, move it to the existing TombEngine style:

```text
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "WeatherQuality", WEATHER_QUALITIES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereEffectType", ATMOSPHERE_EFFECT_TYPES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereEffectScope", ATMOSPHERE_EFFECT_SCOPES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereEffectRenderMode", ATMOSPHERE_EFFECT_RENDER_MODES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereEnvironmentMode", ATMOSPHERE_ENVIRONMENT_MODES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmospherePrecipitationMaterial", ATMOSPHERE_PRECIPITATION_MATERIALS)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereCelestialBodyType", ATMOSPHERE_CELESTIAL_BODY_TYPES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereCelestialRenderMode", ATMOSPHERE_CELESTIAL_RENDER_MODES)
```

## Current Pass Criteria

This branch is ready for the next implementation step when:

```text
- TombEngine compiles locally.
- Existing legacy weather scripts still load.
- A level script with level.atmosphere loads.
- A level script with level.atmosphereEnvironment loads.
- A level script with level.atmosphereCelestial loads.
- MoonProfile, LightShaftProfile, and AtmosphereCelestialBodyProfile load as data.
- WeatherQuality and AtmosphereEffect* tables are usable in Lua.
- AtmosphereEnvironment* tables are usable in Lua.
- AtmosphereCelestial* tables are usable in Lua.
- The .vcxproj diff is cleaned before PR.
```
