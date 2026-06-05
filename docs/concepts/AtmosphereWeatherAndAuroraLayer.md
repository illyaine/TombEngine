# Atmosphere, Weather, and Aurora Layer Concept

## Purpose

This document describes a clean technical direction for a new TombEngine atmosphere layer that can support Aurora Borealis, modernized weather rendering, weather interaction, generated atmosphere effects, and future sky effects without extending the current sprite-heavy weather path further.

The goal is not to replace existing level scripts or break current projects. The goal is to provide a compatible foundation that feels native to TombEngine, follows the existing Flow/Lua style, and can be introduced gradually.

## Current Context

The existing Flow level API exposes weather through a small set of level fields:

```lua
level.weather = WeatherType.Rain
level.weatherStrength = 1.0
level.weatherClustering = true
```

That style should remain valid. Existing levels should continue to run without script changes.

The current weather implementation is part of the environment controller together with sky, storm, wind, flash, starfield, meteors, lens flare, and existing effects. Rain and snow are simulated as weather particles and later prepared as sprites. Dense weather can therefore become expensive when combined with other sprite-like effects such as stars, meteors, fire, smoke, ripples, sparks, and custom particles.

Aurora Borealis should not be forced into the existing horizon mesh, sky texture scroll, or weather sprite path. Previous experiments showed that this does not produce a clean enough result. Aurora needs its own sky/atmosphere render layer.

## Design Goals

- Keep all user-facing configuration available through Flow/Lua.
- Preserve the existing `level.weather`, `level.weatherStrength`, and `level.weatherClustering` fields.
- Add new atmosphere configuration in the same style as existing Flow objects.
- Prefer engine-generated atmosphere layers over mandatory external sprite assets.
- Allow optional sprite or existing-effect paths only when a builder explicitly wants them.
- Avoid visible hardcoded text.
- Keep the implementation close to TombEngine's existing style and naming patterns.
- Avoid a large enterprise-style abstraction layer.
- Keep renderer changes incremental and reviewable.
- Do not route mass weather rendering through the general sprite sorting path.
- Make Aurora a first-class sky effect, not a weather particle effect.
- Add quality budgets so dense weather remains controllable.
- Prepare future generated atmosphere types such as falling leaves, cherry blossoms, ash, sand, dust, snowstorms, sandstorms, and custom magic particles.
- Do not duplicate already existing effect systems. Existing effects such as fireflies can be connected later through explicit bridge or custom render modes.

## Proposed Runtime Structure

```text
EnvironmentController
 ├─ Legacy weather compatibility
 ├─ AtmosphereController
 │   ├─ WeatherController
 │   ├─ WindController
 │   ├─ SkyEffectController
 │   ├─ GeneratedEffectController
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
 ├─ Generated atmosphere effects
 │   ├─ Ground fog / mist layers
 │   ├─ Leaves / ash / dust layers
 │   ├─ Snowstorm / sandstorm veil layers
 │   └─ Custom generated layers
 ├─ Weather instanced rendering
 │   ├─ Rain instances
 │   ├─ Snow instances
 │   └─ Future custom weather instances
 ├─ Optional existing effect bridges
 ├─ Existing sprite/effect rendering
 └─ Post effects
```

Aurora should render as a sky/atmosphere layer. Rain and snow should eventually render through dedicated instanced weather buffers instead of being expanded into many general-purpose sprite entries.

Generated atmosphere effects should not require external sprites by default. They may use procedural shapes, camera-facing generated quads, volume slices, generated noise, instancing, or a later GPU path. External sprite textures should remain optional for builders who explicitly want a specific custom look.

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
Sand
GeneratedCustom
ExistingEffectBridge
```

The initial implementation should focus only on preparing the architecture and not attempt to finish every layer immediately.

## Meshless and Volumetric Atmosphere Layers

Some atmosphere effects should not require level builders to place visible objects or many nullmeshes. They should be defined as atmosphere layers and optionally use placed objects only as emitters, anchors, or zone markers.

Recommended generated layer examples:

```text
GroundFog
DriftingMist
SnowstormVeil
SandstormVeil
DustSheet
AshFall
LeafFall
MagicParticles
Custom
```

Existing effects that already exist in TombEngine, such as firefly-style effects, should not be duplicated blindly. The atmosphere system can later expose a bridge or preset that drives an existing effect system when this is cleaner than reimplementing it.

Renderer notes:

- Keep generated effects outside the general weather sprite sorting path where possible.
- Support global layers and later room/zone-limited layers.
- Allow nullmesh or trigger objects to act as optional emitters, not as mandatory visual meshes.
- Use engine-generated visuals by default.
- Use external sprite textures only when `renderMode = AtmosphereEffectRenderMode.Sprite` is explicitly selected.
- Keep quality budgets for density, update cost, and draw distance.
- Prefer generated camera-facing quads, volume slices, instancing, shader noise, or a later GPU path depending on effect type.

This allows effects such as moving ground fog, heavy snowstorm haze, sandstorm sheets, ash fall, falling leaves, or localized magical atmosphere without forcing every effect into moveables, horizon meshes, or classic weather particles.

## Effect Scope and Geometry Blocking

Atmosphere effects should support both global use and localized use.

Recommended scopes:

```text
Global    - effect applies broadly to the level or outdoor area.
Nullmesh  - effect is emitted around a named nullmesh/object anchor.
Room      - effect is constrained to a room or room group.
Volume    - effect is constrained to a later explicit atmosphere volume.
```

Movement and spawning must respect level geometry wherever possible. Effects such as leaves, ash, dust, mist, or ground fog should not visibly pass through walls, floors, or ceilings.

Recommended collision flags:

```text
collideWithGeometry = true
stopAtWalls = true
stopAtFloors = true
stopAtCeilings = true
clampToRoom = true
```

The defaults should be safe. A builder may later relax these rules for special effects, but normal atmosphere should stop at room boundaries and solid geometry.

## Generated vs Sprite Rendering

The default render mode should be generated:

```lua
renderMode = AtmosphereEffectRenderMode.Generated
```

Generated mode means the engine creates the visible layer internally from profile data such as density, size range, seed, color, softness, detail, speed, direction, turbulence, and wind influence. The builder should not be forced to provide sprite assets for common atmosphere layers.

Optional modes:

```text
Generated      - default. Engine-generated visual layer.
Sprite         - optional external sprite/texture path for custom builder art.
ExistingEffect - future bridge to an existing engine effect, not a duplicate implementation.
Custom         - future custom/plugin or renderer-specific path.
```

This keeps the system open without hardcoding every future effect into one closed list.

## Weather Budgets

Dense weather needs explicit budgets. These budgets should be script-configurable, but safe defaults should exist.

Recommended fields:

```text
maxRainInstances
maxSnowInstances
maxWeatherInstances
maxGeneratedAtmosphereInstances
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
	},

	effects = {
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
			colorB = Color(90, 60, 20),
			stopAtWalls = true,
			stopAtFloors = true,
			stopAtCeilings = true,
			clampToRoom = true
		},

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
			clampToRoom = true
		}
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
WeatherQuality.Low
WeatherQuality.Medium
WeatherQuality.High
WeatherQuality.Ultra
WeatherQuality.Auto

AtmosphereEffectType.None
AtmosphereEffectType.GroundFog
AtmosphereEffectType.Mist
AtmosphereEffectType.SnowstormVeil
AtmosphereEffectType.SandstormVeil
AtmosphereEffectType.DustSheet
AtmosphereEffectType.AshFall
AtmosphereEffectType.LeafFall
AtmosphereEffectType.MagicParticles
AtmosphereEffectType.Custom

AtmosphereEffectScope.Global
AtmosphereEffectScope.Nullmesh
AtmosphereEffectScope.Room
AtmosphereEffectScope.Volume

AtmosphereEffectRenderMode.Generated
AtmosphereEffectRenderMode.Sprite
AtmosphereEffectRenderMode.ExistingEffect
AtmosphereEffectRenderMode.Custom

SkyEffectType.None
SkyEffectType.Aurora
```

Only the required enum values should be added during implementation. The list above is the long-term direction. New concrete effect behavior should be added as generated presets or effect bridges where possible, not by duplicating existing systems.

## Implementation Phases

### Phase 1: Concept and Compatibility

- Add this concept document.
- Do not change runtime behavior.
- Keep existing weather behavior untouched.

### Phase 2: Lua Data Shape

- Add Flow data classes for `Atmosphere`, `WeatherProfile`, `WindProfile`, `AuroraProfile`, and generated atmosphere effect profiles.
- Keep old fields functional.
- Add getters to the level script interface only where needed.
- Do not activate new rendering yet.

### Phase 3: Aurora Render Layer

- Add a dedicated aurora shader.
- Add a small renderer path for the aurora layer.
- Render it as a sky/atmosphere effect.
- Keep it independent from horizon mesh hacks and weather sprites.

### Phase 4: Generated Atmosphere Effects

- Add a small generated effect controller.
- Start with one safe layer such as ground fog or leaf fall.
- Respect rooms, walls, floors, and ceilings before exposing dense effects.
- Keep sprite textures optional and disabled by default.

### Phase 5: Weather Budgets

- Add counters and budgets for current weather.
- Add debug counters internally.
- Reduce excessive impact spawning first.

### Phase 6: Instanced Weather Rendering

- Add a dedicated weather instance buffer.
- Move rain/snow rendering away from the general sprite sorting path.
- Keep existing simulation until the renderer path is stable.

### Phase 7: Extended Weather Types and Bridges

- Add generated leaves, cherry blossoms, ash, dust, sand, and storm variants incrementally.
- Add bridges for existing effects only where useful.
- Keep each layer optional and budgeted.

## Notes for Pull Request Positioning

This concept can cover several existing community requests without treating each feature as an isolated hack:

- Aurora Borealis.
- Better weather interaction.
- Rain affected by wind.
- Falling leaves and cherry blossoms.
- Generated fog, dust, ash, snowstorm, and sandstorm layers.
- Optional bridges for existing effects such as fireflies.
- Better water/weather impact effects.
- Better performance behavior for dense weather.

The first real code change should be intentionally small: add the Flow data shape and disabled generated effect profiles, then activate rendering only when the runtime path is ready.
