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
- Flow.AtmosphereEffectProfile data shape.
- AtmosphereRuntimeSnapshot resolved global runtime data.
- AtmosphereRuntimeController non-rendering shell.
- Level::CreateAtmosphereRuntimeSnapshot() helper.
- Backward-compatible Level weather fallback.
- Enabled/local effect counting helpers.
- Documentation for Tomb Editor / TombEngine boundary.
- Documentation for generated effects and editor-driven local emitter configuration.
- Test plan for local compile/script loading checks.
```

Not implemented yet:

```text
- Runtime GeneratedEffectController.
- Aurora renderer layer.
- Weather instanced renderer path.
- Local/nullmesh emitter runtime activation.
- Geometry collision for generated atmosphere effects.
- Tomb Editor Object Parameter System integration.
```

## Script Boundary

Manual Flow/Lua should primarily cover global atmosphere configuration:

```text
- Global weather.
- Global rain/snow quality and budgets.
- Global wind.
- Global aurora.
- General level-wide atmosphere defaults.
```

Local/nullmesh/room/volume effect tuning should be authored later through the Tomb Editor Object Parameter System and exported as generated Flow data or runtime metadata.

This prevents normal builders from having to write large `Flow.AtmosphereEffectProfile` blocks manually.

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
EnabledEffectCount
LocalEffectCount
```

Current fallback rule:

```text
If level.atmosphere.enabled is false, the snapshot uses legacy level.weather, level.weatherStrength, and level.weatherClustering.
If level.atmosphere.enabled is true, the snapshot uses level.atmosphere.weather, level.atmosphere.wind, and level.atmosphere.aurora.
```

Local effect profiles are counted but not activated by runtime renderer code yet.

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
HasEnabledEffects()
HasLocalEffects()
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
- Aurora does not render yet.
- Generated effects do not render yet.
- Local/nullmesh effects do not render yet.
```

## Known Review Notes

The current branch still needs local compile verification.

Do not claim build success without an actual build.

Areas to check during local compile:

```text
- sol::table::new_enum support in this TombEngine sol2 setup.
- std::vector<AtmosphereEffectProfile> Lua binding behavior.
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
4. Add Aurora renderer planning stubs only after Flow data is stable.
5. Add one simple generated global layer before local/nullmesh runtime activation.
6. Enable local/nullmesh emitters only after Tomb Editor can author object parameters cleanly.
```

## PR Positioning

This branch should be presented as an incremental foundation, not as a finished weather renderer.

Recommended wording:

```text
- Adds an opt-in atmosphere profile data layer.
- Preserves existing weather fields.
- Adds a non-rendering runtime snapshot/controller read path.
- Documents the future editor-driven local emitter workflow.
- Keeps local/nullmesh detailed tuning out of normal hand-written scripts.
- Prepares generated effects and aurora without forcing renderer activation yet.
```
