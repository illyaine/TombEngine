# Atmosphere System Current State

## Branch

```text
atmosphere_system
```

## Current Scope

This branch currently prepares the data, script, and non-rendering runtime foundation for a TombEngine atmosphere system.

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
- AtmosphereRuntimeSnapshot resolved global runtime data.
- AtmosphereRuntimeController non-rendering shell.
- Level::CreateAtmosphereRuntimeSnapshot() helper.
- Backward-compatible Level weather fallback.
- Enabled/local effect and light shaft counting helpers.
- Documentation for Tomb Editor / TombEngine boundary.
- Documentation for generated effects and editor-driven local emitter configuration.
- Test plan for local compile/script loading checks.
```

Not implemented yet:

```text
- Runtime GeneratedEffectController.
- Aurora renderer layer.
- Moon renderer layer.
- Light shaft renderer layer.
- Weather instanced renderer path.
- Local/nullmesh emitter runtime activation.
- Geometry collision for generated atmosphere effects and light shafts.
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
- Global light shafts / god rays.
- General level-wide atmosphere defaults.
```

Local/nullmesh/room/volume effect tuning should be authored later through the Tomb Editor Object Parameter System and exported as generated Flow data or runtime metadata.

This prevents normal builders from having to write large `Flow.AtmosphereEffectProfile` or `Flow.LightShaftProfile` blocks manually.

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

The current branch now has a non-rendering snapshot helper:

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
```

Local effect and light shaft profiles are counted but not activated by runtime renderer code yet.

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
- Moon pitch/yaw data can be loaded.
- Global and nullmesh/anchor light shaft data can be loaded.
- Aurora does not render yet.
- Moon does not render yet.
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
- Inline AtmosphereRuntimeSnapshot / AtmosphereRuntimeController helper compatibility with current include order.
- Atmosphere file entries in TombEngine.vcxproj.
- TombEngine.vcxproj line-ending/noisy diff cleanup before PR.
```

## Suggested Next Safe Runtime Steps

Recommended sequence after local compile is available:

```text
1. Fix compile issues from the current Flow data shape if any.
2. Clean TombEngine.vcxproj to avoid large noisy diff.
3. Wire AtmosphereRuntimeController into the real environment update path without rendering.
4. Add moon renderer planning stubs and then a simple sky billboard/quad layer.
5. Add Aurora renderer planning stubs after Flow data is stable.
6. Add one simple generated global layer before local/nullmesh runtime activation.
7. Enable local/nullmesh emitters and light shafts only after Tomb Editor can author object parameters cleanly.
```

## PR Positioning

This branch should be presented as an incremental foundation, not as a finished weather renderer.

Recommended wording:

```text
- Adds an opt-in atmosphere profile data layer.
- Preserves existing weather fields.
- Adds a non-rendering runtime snapshot/controller read path.
- Adds script data for aurora, moon, generated effects, and light shafts.
- Documents the future editor-driven local emitter workflow.
- Keeps local/nullmesh detailed tuning out of normal hand-written scripts.
- Prepares generated effects and celestial sky effects without forcing renderer activation yet.
```
