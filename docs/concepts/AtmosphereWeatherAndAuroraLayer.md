# Atmosphere, Weather, and Aurora Layer Concept

## Purpose

This document describes a clean technical direction for a new TombEngine atmosphere layer that can support Aurora Borealis, modernized weather rendering, weather interaction, and future sky effects without extending the current sprite-heavy weather path further.

The goal is not to replace existing level scripts or break current projects. The goal is to provide a compatible foundation that feels native to TombEngine, follows the existing Flow/Lua style, and can be introduced gradually.

## Current Context

The existing Flow level API exposes weather through a small set of level fields:

```lua
level.weather = WeatherType.Rain
level.weatherStrength = 1.0
level.weatherClustering = true
```

That style should remain valid. Existing levels should continue to run without script changes.

The current weather implementation is part of the environment controller together with sky, storm, wind, flash, starfield, meteors, and lens flare. Rain and snow are simulated as weather particles and later prepared as sprites. Dense weather can therefore become expensive when combined with other sprite-like effects such as stars, meteors, fire, smoke, ripples, sparks, and custom particles.

Aurora Borealis should not be forced into the existing horizon mesh, sky texture scroll, or weather sprite path. Previous experiments showed that this does not produce a clean enough result. Aurora needs its own sky/atmosphere render layer.

## Design Goals

- Keep all user-facing configuration available through Flow/Lua.
- Preserve the existing `level.weather`, `level.weatherStrength`, and `level.weatherClustering` fields.
- Add new atmosphere configuration in the same style as existing Flow objects.
- Avoid visible hardcoded text.
- Keep the implementation close to TombEngine's existing style and naming patterns.
- Avoid a large enterprise-style abstraction layer.
- Keep renderer changes incremental and reviewable.
- Do not route mass weather rendering through the general sprite sorting path.
- Make Aurora a first-class sky effect, not a weather particle effect.
- Add quality budgets so dense weather remains controllable.
- Prepare future weather types such as falling leaves, cherry blossoms, ash, sand, dust, fireflies, snowstorms, and sandstorms.

## Proposed Runtime Structure

```text
EnvironmentController
 ├─ Legacy weather compatibility
 ├─ AtmosphereController
 │   ├─ WeatherController
 │   ├─ WindController
 │   ├─ SkyEffectController
 │   └─ WeatherImpactController
 └─ Existing sky, storm, flash, starfield, lens flare compatibility
```

The structure should be introduced carefully. It does not need to be implemented as many new services or interfaces at once. The first implementation can be small and concrete.

## Proposed Renderer Structure

```text
Renderer
 ├─ Existing world rendering
 ├─ Existing sky and horizon rendering
 ├─ Atmosphere sky effects
 │   └─ Aurora layer
 ├─ Weather instanced rendering
 │   ├─ Rain instances
 │   ├─ Snow instances
 │   └─ Future custom weather instances
 ├─ Existing sprite/effect rendering
 └─ Post effects
```

Aurora should render as a sky/atmosphere layer. Rain and snow should eventually render through dedicated instanced weather buffers instead of being expanded into many general-purpose sprite entries.

## Aurora Layer

Aurora should be implemented as a dedicated sky effect layer.

Recommended properties:

```text
enabled
intensity
speed
height
width
waveScale
waveStrength
colorA
colorB
colorC
transparency
fadeWithFog
```

Renderer notes:

- Use a dedicated shader.
- Use procedural noise or scrolling bands in shader space.
- Render after base sky and before foreground weather/effects.
- Avoid using horizon mesh UV hacks.
- Avoid requiring a custom level mesh for the aurora itself.
- Keep the first version simple and stable.

## Weather Rendering

The long-term weather path should use GPU-friendly instanced rendering.

Recommended instance data:

```cpp
struct RendererWeatherInstance
{
	Vector3 Position;
	Vector3 Velocity;
	float Size;
	float Alpha;
	float Rotation;
	float AnimationFrame;
	int Type;
};
```

Recommended render layers:

```text
Rain
Snow
Dust
Ash
Leaves
Fireflies
Sand
Custom
```

The initial implementation should focus only on preparing the architecture and not attempt to finish every layer immediately.

## Meshless and Volumetric Atmosphere Layers

Some atmosphere effects should not require level builders to place visible objects or many nullmeshes. They should be defined as atmosphere layers and optionally use placed objects only as emitters, anchors, or zone markers.

Recommended future layer examples:

```text
GroundFog
DriftingMist
SnowstormVeil
SandstormVeil
DustSheet
AshFall
LeafFall
Fireflies
MagicParticles
```

Renderer notes:

- Keep these effects outside the general weather sprite sorting path where possible.
- Support global layers and later room/zone-limited layers.
- Allow nullmesh or trigger objects to act as optional emitters, not as mandatory visual meshes.
- Keep quality budgets for density, update cost, and draw distance.
- Prefer camera-facing quads, volume slices, instancing, or a later GPU path depending on effect type.

This allows effects such as moving ground fog, heavy snowstorm haze, sandstorm sheets, ash fall, or localized magical atmosphere without forcing every effect into moveables, horizon meshes, or classic weather particles.

## Weather Budgets

Dense weather needs explicit budgets. These budgets should be script-configurable, but safe defaults should exist.

Recommended fields:

```text
maxRainInstances
maxSnowInstances
maxWeatherInstances
maxImpactsPerFrame
maxRipplesPerFrame
maxSparkEffectsPerFrame
autoReduceDensity
```

The renderer should be able to reduce far weather density before reducing near weather density.

## Weather LOD

Weather should use separate density behavior for near, mid, and far ranges.

```text
Near range:
- more individual visible drops/flakes
- stronger motion detail
- optional impact events

Mid range:
- fewer particles
- larger streaks or flakes
- lower update cost

Far range:
- atmospheric sheet/noise impression
- minimal or no individual particles
- no impact events
```

This keeps weather visually dense without requiring extremely high real particle counts.

## Weather Impacts

Rain impacts, ripples, and water sparks should not be spawned for every rain particle.

Recommended behavior:

- Budget impact events per frame.
- Prefer nearby visible impacts.
- Skip far impacts.
- Allow water, swamp, floor, and object impacts to use different behavior later.
- Keep Lua options for enabling/disabling impact groups.

## Wind

Wind should affect rain, snow, leaves, ash, dust, and sand through the same wind profile.

Recommended fields:

```text
direction
strength
gustStrength
gustFrequency
turbulence
verticalDrift
```

Rain wind influence should be optional for compatibility with existing levels.

## Proposed Flow/Lua API

The new API should follow the existing Flow object style rather than introducing a separate script system.

Example:

```lua
level.atmosphere = Flow.Atmosphere {
	enabled = true,

	weather = Flow.WeatherProfile {
		type = WeatherType.Rain,
		strength = 0.85,
		clustering = true,
		quality = WeatherQuality.Auto,

		rain = Flow.RainProfile {
			windInfluence = 0.75,
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
		turbulence = 0.2
	},

	aurora = Flow.AuroraProfile {
		enabled = true,
		intensity = 0.7,
		speed = 0.25,
		height = 0.8,
		colorA = Color(80, 180, 255),
		colorB = Color(120, 255, 180),
		colorC = Color(180, 120, 255)
	}
}
```

Legacy-compatible scripts remain valid:

```lua
level.weather = WeatherType.Rain
level.weatherStrength = 1.0
level.weatherClustering = true
```

The new profile is opt-in. As long as `level.atmosphere.enabled` remains false, TombEngine should continue to use the legacy `level.weather`, `level.weatherStrength`, and `level.weatherClustering` fields. When `level.atmosphere.enabled` is true, runtime code can read weather settings from the atmosphere profile while keeping the old fields available for compatibility.

## Proposed Enums

```text
WeatherType.None
WeatherType.Rain
WeatherType.Snow
WeatherType.Leaves
WeatherType.Ash
WeatherType.Dust
WeatherType.Sand
WeatherType.Fireflies

WeatherQuality.Low
WeatherQuality.Medium
WeatherQuality.High
WeatherQuality.Ultra
WeatherQuality.Auto

SkyEffectType.None
SkyEffectType.Aurora
```

Only the required enum values should be added during implementation. The list above is the long-term direction.

## Implementation Phases

### Phase 1: Concept and Compatibility

- Add this concept document.
- Do not change runtime behavior.
- Keep existing weather behavior untouched.

### Phase 2: Lua Data Shape

- Add Flow data classes for `Atmosphere`, `WeatherProfile`, `WindProfile`, and `AuroraProfile`.
- Keep old fields functional.
- Add getters to the level script interface only where needed.
- Do not activate new rendering yet.

### Phase 3: Aurora Render Layer

- Add a dedicated aurora shader.
- Add a small renderer path for the aurora layer.
- Render it as a sky/atmosphere effect.
- Keep it independent from horizon mesh hacks and weather sprites.

### Phase 4: Weather Budgets

- Add counters and budgets for current weather.
- Add debug counters internally.
- Reduce excessive impact spawning first.

### Phase 5: Instanced Weather Rendering

- Add a dedicated weather instance buffer.
- Move rain/snow rendering away from the general sprite sorting path.
- Keep existing simulation until the renderer path is stable.

### Phase 6: Extended Weather Types

- Add leaves, cherry blossoms, ash, dust, sand, fireflies, and storm variants incrementally.
- Keep each layer optional and budgeted.

## Notes for Pull Request Positioning

This concept can cover several existing community requests without treating each feature as an isolated hack:

- Aurora Borealis.
- Better weather interaction.
- Rain affected by wind.
- Falling leaves and cherry blossoms.
- Fireflies, dust, snowstorms, and sandstorms.
- Better water/weather impact effects.
- Better performance behavior for dense weather.

The first real code change should be intentionally small: add the Flow data shape and a disabled aurora layer path, then activate the layer only when explicitly configured.
