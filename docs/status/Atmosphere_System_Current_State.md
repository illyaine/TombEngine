# Atmosphere System Current State

## Branch

```text
atmosphere_system
```

## Current Scope

This branch currently prepares the data, script, non-rendering runtime foundation, renderer-facing data foundation, environment policy layer, and celestial render activation layer for a TombEngine atmosphere system.

Implemented scope:

```text
- Flow.Atmosphere data shape.
- Flow.WeatherProfile data shape.
- Flow.RainProfile data shape.
- Flow.WindProfile data shape.
- Flow.AuroraProfile data shape.
- Flow.MoonProfile data shape with Lua pitch/yaw positioning.
- Flow.AtmosphereEffectProfile data shape.
- Flow.LightShaftProfile data shape for global and nullmesh/anchor-driven shafts.
- Flow.AtmosphereEnvironmentProfile data shape for space/indoor/underwater/toxic/alien weather policy.
- Flow.AtmosphereCelestialProfile data shape for planets, moons, stars, comets, debris, nebulae, and galaxy bands.
- Flow.AtmosphereCelestialBodyProfile data shape for individual celestial objects.
- AtmosphereRuntimeSnapshot resolved global runtime data.
- AtmosphereRenderData renderer-facing filtered data container.
- AtmosphereCelestialRenderData renderer-facing celestial activation container.
- AtmosphereRuntimeController non-rendering shell with snapshot and render data cache.
- Level::CreateAtmosphereRuntimeSnapshot() helper.
- Level::CreateAtmosphereRenderData() helper.
- Level::CreateAtmosphereRenderPlan() helper.
- Level::CreateAtmosphereCelestialRenderData() helper.
- Level::CreateSkyAtmosphereRenderData() helper with atmosphere and celestial activation payloads.
- Backward-compatible Level weather fallback.
- Environment policy filtering for weather and wind read paths.
- Enabled/local effect and light shaft counting helpers.
- Celestial activation bucket counts for future sky-object renderers.
- Documentation for Tomb Editor / TombEngine boundary.
- Documentation for generated effects and editor-driven local emitter configuration.
- Documentation for celestial render activation.
- Test plan for local compile/script loading checks.
```

Not implemented yet:

```text
- Runtime GeneratedEffectController.
- Aurora renderer layer.
- Moon renderer layer.
- Planet / celestial 3D sphere renderer layer.
- Celestial billboard renderer layer.
- Celestial generated galaxy/nebula layer renderer.
- Light shaft renderer layer.
- Weather instanced renderer path.
- Local/nullmesh emitter runtime activation.
- Geometry collision for generated atmosphere effects and light shafts.
- Gameplay damage loop for toxic precipitation.
- Tomb Editor Object Parameter System integration.
```

## Script Boundary

Manual Flow/Lua should primarily cover global atmosphere configuration:

```text
- Global weather.
- Global rain/snow quality and budgets.
- Global wind.
- Global aurora.
- Global moon position and moonlight.
- Global celestial sky stack presets such as planet/moon/comet/galaxy layers.
- Global light shafts / god rays.
- General level-wide atmosphere defaults.
```

Local/nullmesh/room/volume effect tuning should be authored later through the Tomb Editor Object Parameter System and exported as generated Flow data or runtime metadata.

This prevents normal builders from having to write large `Flow.AtmosphereEffectProfile`, `Flow.LightShaftProfile`, or celestial body arrays manually.

## Moon Positioning

Moon position is script-controlled through sky angles, not world-space position:

```lua
level.atmosphere.moon = Flow.MoonProfile {
	enabled = true,
	pitch = -18,
	yaw = 225,
	size = 1.2,
	intensity = 0.85
}
```

This follows the same general idea as existing sky/lens-flare direction settings and avoids room-dependent coordinates.

## Celestial Sky Stack

The branch now also supports a separate celestial data stack:

```lua
level.atmosphereCelestial = Flow.AtmosphereCelestialProfile {
	enabled = true,
	bodies = {
		Flow.AtmosphereCelestialBodyProfile {
			enabled = true,
			type = Flow.AtmosphereCelestialBodyType.Planet,
			renderMode = Flow.AtmosphereCelestialRenderMode.Sphere3D,
			name = "earth",
			textureName = "earth_day",
			pitch = -12,
			yaw = 210,
			size = 1.4,
			intensity = 1.0
		}
	}
}
```

Supported body types:

```text
Planet
Moon
Star
Comet
Asteroid
SpaceDebris
Nebula
GalaxyBand
Custom
```

Supported render mode hints:

```text
Billboard
Sphere3D
HorizonObject
GeneratedLayer
Custom
```

These are activation hints only. No visible celestial draw code is active yet.

## Celestial Render Activation

The branch now has a non-drawing celestial render activation container:

```cpp
AtmosphereCelestialRenderData celestialData = level.CreateAtmosphereCelestialRenderData();
SkyAtmosphereRenderData skyData = level.CreateSkyAtmosphereRenderData();
```

The activation container classifies enabled bodies into renderer-relevant buckets:

```text
BodyCount
SpaceBodyCount
Sphere3DBodyCount
BillboardBodyCount
HorizonObjectBodyCount
GeneratedLayerCount
LightSourceCount
HideLegacyMoon
HideStarfield
```

This is intended to let later renderer code make simple decisions without reading Flow or Lua objects directly:

```text
Sphere3DBodyCount > 0        -> future 3D planet/moon pass
BillboardBodyCount > 0       -> future flat sky body pass
HorizonObjectBodyCount > 0   -> future existing horizon bridge
GeneratedLayerCount > 0      -> future galaxy/nebula layer pass
LightSourceCount > 0         -> future light direction/color bridge
```

## Environment Policy Layer

The branch now has an environment policy profile:

```lua
level.atmosphereEnvironment = Flow.AtmosphereEnvironmentProfile {
	enabled = true,
	mode = Flow.AtmosphereEnvironmentMode.SpaceVacuum,
	precipitationMaterial = Flow.AtmospherePrecipitationMaterial.None
}
```

Current conservative runtime policy:

```text
- Disabled environment profile keeps legacy behavior.
- Space/underwater/indoor/underground modes can block normal precipitation.
- Space vacuum can block wind unless artificial weather or force flags allow it.
- Hazardous precipitation is blocked unless toxic weather or force precipitation is explicitly allowed.
- Policy currently filters weather/wind read paths and render data.
- No gameplay damage loop is active yet.
```

## Light Shafts

Light shafts support two intended authoring paths:

```text
Global: configured directly through Flow/Lua as level-wide god rays or moon shafts.
Local/nullmesh: configured later through Tomb Editor Object Parameters and exported as data.
```

Local/nullmesh shafts already have a data shape through:

```text
Flow.LightShaftProfile.scope = AtmosphereEffectScope.Nullmesh
Flow.LightShaftProfile.anchorName = "shaft_window_01"
```

They are not rendered yet.

## Object Parameter System Dependency

The detailed local emitter workflow depends on a later Tomb Editor feature:

```text
object_parameter_system
```

That system should expose object-specific structured parameters instead of relying on raw OCB codes.

The TombEngine side must not depend directly on Tomb Editor. The bridge should stay data-based:

```text
Tomb Editor Object Parameters
        ↓ export
Flow.Atmosphere / generated atmosphere metadata
        ↓ runtime
TombEngine atmosphere runtime code
```

## Runtime Snapshot

The current branch has a non-rendering snapshot helper:

```cpp
AtmosphereRuntimeSnapshot snapshot = level.CreateAtmosphereRuntimeSnapshot();
```

The snapshot resolves the active global runtime state without forcing renderer code to read Flow structures directly.

Current snapshot data:

```text
Enabled
Type
Strength
Clustering
Quality
Rain
Wind
Aurora
Moon
EnabledEffectCount
LocalEffectCount
EnabledLightShaftCount
LocalLightShaftCount
```

Current fallback rule:

```text
If level.atmosphere.enabled is false, the snapshot uses legacy level.weather, level.weatherStrength, and level.weatherClustering.
If level.atmosphere.enabled is true, the snapshot uses level.atmosphere.weather, level.atmosphere.wind, level.atmosphere.aurora, level.atmosphere.moon, and light shaft/effect counts.
Environment policy is applied after the snapshot is created.
```

Local effect and light shaft profiles are counted but not activated by runtime renderer code yet.

## Render Data

The branch has renderer-facing atmosphere data without visible renderer activation:

```cpp
AtmosphereRenderData renderData = level.CreateAtmosphereRenderData();
```

The render data collects only active, render-relevant payloads:

```text
Enabled
WeatherTypeValue
WeatherStrength
WeatherClustering
Quality
Rain
Wind
Aurora
Moon
Effects               active effects only
LightShafts           active light shafts only
```

Available render-data checks:

```text
HasWeather()
HasAurora()
HasMoon()
HasEffects()
HasLocalEffects()
HasLightShafts()
HasLocalLightShafts()
```

This is still non-drawing data. No draw calls, shaders, GPU resources, or visible layers are created by this branch yet.

## Non-Rendering Controller

The branch also includes a small non-rendering controller shell:

```cpp
AtmosphereRuntimeController controller;
controller.Update(level.GetAtmosphere(), level.Weather, level.WeatherStrength, level.WeatherClustering);
```

Available checks:

```text
IsEnabled()
HasWeather()
HasAurora()
HasMoon()
HasEnabledEffects()
HasLocalEffects()
HasLightShafts()
HasLocalLightShafts()
GetSnapshot()
GetRenderData()
Reset()
```

This is intentionally renderer-neutral. It is only a stable read/cache path for the next runtime phase.

## Backward Compatibility

Existing legacy script fields remain valid:

```lua
level.weather = WeatherType.Rain
level.weatherStrength = 1.0
level.weatherClustering = true
```

Current fallback rule for existing getters:

```text
If level.atmosphere.enabled is false, runtime getters continue to use legacy level.weather, level.weatherStrength, and level.weatherClustering.
If level.atmosphere.enabled is true, runtime getters can read weather data from the atmosphere profile.
Environment policy can still block or modify the final returned weather/wind data.
```

## Tonight's Test Scope

Use:

```text
docs/testing/Atmosphere_System_Test_Plan.md
```

Current expected test result:

```text
- Legacy weather still works.
- level.atmosphere can be added as data if Flow binding compiles.
- level.atmosphereEnvironment can be added as data if Flow binding compiles.
- level.atmosphereCelestial can be added as data if Flow binding compiles.
- Moon pitch/yaw data can be loaded.
- Celestial body data can be loaded.
- Global and nullmesh/anchor light shaft data can be loaded.
- RenderData can be created from Level after compile.
- CelestialRenderData can be created from Level after compile.
- Aurora does not render yet.
- Moon does not render yet.
- Planets/celestial bodies do not render yet.
- Light shafts do not render yet.
- Generated effects do not render yet.
- Local/nullmesh effects do not render yet.
```

## Known Review Notes

The current branch still needs local compile verification.

Do not claim build success without an actual build.

Areas to check during local compile:

```text
- Flow enum table registration compatibility.
- std::vector<AtmosphereEffectProfile> Lua binding behavior.
- std::vector<LightShaftProfile> Lua binding behavior.
- std::vector<AtmosphereCelestialBodyProfile> Lua binding behavior.
- Inline AtmosphereRuntimeSnapshot / AtmosphereRenderData / AtmosphereRuntimeController helper compatibility with current include order.
- Inline AtmosphereCelestialRenderData helper compatibility with current include order.
- Atmosphere file entries in TombEngine.vcxproj.
- TombEngine.vcxproj line-ending/noisy diff cleanup before PR.
```

## Suggested Next Safe Runtime Steps

Recommended sequence after local compile is available:

```text
1. Fix compile issues from the current Flow data shape if any.
2. Clean TombEngine.vcxproj to avoid large noisy diff.
3. Wire SkyAtmosphereRenderData into the real renderer/environment update path without drawing.
4. Add one minimal renderer-side activation check for celestial body passes.
5. Add one simple billboard sky object path.
6. Add Sphere3D body path for moon/planet rendering.
7. Add generated galaxy/nebula layer after object rendering is stable.
8. Add Aurora as a separate atmosphere layer, not as a Horizon/Sky.hlsl hack.
9. Add local/nullmesh emitters and light shafts only after Tomb Editor can author object parameters cleanly.
```

## PR Positioning

This branch should be presented as an incremental foundation, not as a finished weather renderer.

Recommended wording:

```text
- Adds an opt-in atmosphere profile data layer.
- Preserves existing weather fields.
- Adds a non-rendering runtime snapshot/controller read path.
- Adds renderer-facing atmosphere render data without activating draw code.
- Adds script data for aurora, moon, generated effects, light shafts, environment policy, and celestial sky objects.
- Adds a celestial render activation data layer for future planet/moon/comet/galaxy rendering.
- Documents the future editor-driven local emitter workflow.
- Keeps local/nullmesh detailed tuning out of normal hand-written scripts.
- Prepares generated effects and celestial sky effects without forcing visible renderer activation yet.
```
