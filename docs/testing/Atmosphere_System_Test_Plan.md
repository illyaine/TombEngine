# Atmosphere System Test Plan

## Purpose

This file describes what can be tested from the current `atmosphere_system` branch before renderer activation exists.

The current branch is not expected to show aurora or generated atmosphere visuals yet. The goal of this test pass is to verify that the Flow data shape, legacy weather fallback, enum tables, and non-rendering runtime snapshot path compile and do not break existing levels.

## Expected Current Behavior

Expected visible behavior:

```text
- Existing legacy weather should still work.
- Existing levels without level.atmosphere should behave like before.
- level.atmosphere can be present in Flow/Lua without crashing script loading if the binding compiles.
- WeatherQuality, AtmosphereEffectType, AtmosphereEffectScope, and AtmosphereEffectRenderMode should be available as Flow tables.
- Aurora fields should load as data only. They are not rendered yet.
- Local/nullmesh effects should load as data only. They are not rendered yet.
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
	}
}
```

Expected result:

```text
- Script loads if Flow binding is correct.
- Weather getters should use atmosphere.weather values.
- Aurora is not visible yet because no renderer layer is implemented.
```

## Test 3: Generated Effect Data Shape Only

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

## Test 4: Local Effect Data Shape Only

Add one local/nullmesh-style entry only after Test 3 loads:

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

## If Compile Fails

Known likely failure points:

```text
- Flow table registration for WeatherQuality / AtmosphereEffectType / AtmosphereEffectScope / AtmosphereEffectRenderMode may need to move to FlowHandler MakeReadOnlyTable style.
- std::vector<AtmosphereEffectProfile> assignment from Lua table may need a table-wrapper pattern.
- TombEngine.vcxproj noisy diff may need local cleanup.
```

If atmosphere enum table registration fails, move it to the existing TombEngine style:

```text
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "WeatherQuality", WEATHER_QUALITIES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereEffectType", ATMOSPHERE_EFFECT_TYPES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereEffectScope", ATMOSPHERE_EFFECT_SCOPES)
FlowHandler::_handler.MakeReadOnlyTable(tableFlow, "AtmosphereEffectRenderMode", ATMOSPHERE_EFFECT_RENDER_MODES)
```

## Current Pass Criteria

This branch is ready for the next implementation step when:

```text
- TombEngine compiles locally.
- Existing legacy weather scripts still load.
- A level script with level.atmosphere loads.
- WeatherQuality and AtmosphereEffect* tables are usable in Lua.
- The .vcxproj diff is cleaned before PR.
```
