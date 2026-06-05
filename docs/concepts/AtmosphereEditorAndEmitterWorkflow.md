# Atmosphere Editor and Emitter Workflow Concept

## Purpose

This document describes how the TombEngine atmosphere system should be exposed to level builders later in Tomb Editor without forcing them to write large Lua blocks or rely on raw OCB values.

The runtime-facing Flow/Lua API remains useful for compatibility, advanced builders, and generated gameflow output. The normal builder workflow should be editor-driven.

## Naming

The user-facing editor feature should not be named `OCB Editor`.

OCB is a legacy Tomb Raider concept and may disappear or become less central in future editor workflows. A neutral name is better because the same panel can edit normal object parameters, emitter parameters, generated atmosphere profiles, and later plugin-defined properties.

Recommended names:

```text
Object Parameters
Object Settings
Emitter Settings
Effect Profile
Atmosphere Emitter
Atmosphere Source
```

Recommended short-term UI naming:

```text
Object Parameters
```

Recommended atmosphere-specific object type or panel title:

```text
Atmosphere Emitter
```

## Builder Workflow

The builder should be able to create atmosphere effects with minimal Lua.

Recommended workflow:

1. Place an atmosphere emitter object or nullmesh-like anchor in Tomb Editor.
2. Select it.
3. Open `Object Parameters`.
4. Choose an effect preset such as `Ground Fog`, `Leaf Fall`, `Ash Fall`, `Snowstorm Veil`, or `Sandstorm Veil`.
5. Adjust simple fields such as radius, height, density, speed, color, wind influence, and geometry blocking.
6. Tomb Editor exports the matching Flow atmosphere profile automatically.

The builder should not have to write a full Lua block for every local fog pool or leaf emitter.

## Runtime Data Direction

The runtime should receive explicit atmosphere profile data.

Recommended generated Flow shape:

```lua
level.atmosphere = Flow.Atmosphere {
	enabled = true,

	effects = {
		Flow.AtmosphereEffectProfile {
			enabled = true,
			type = AtmosphereEffectType.GroundFog,
			scope = AtmosphereEffectScope.Nullmesh,
			anchorName = "fog_pool_01",
			renderMode = AtmosphereEffectRenderMode.Generated,
			radius = 1536,
			height = 256,
			density = 0.8,
			collideWithGeometry = true,
			stopAtWalls = true,
			stopAtFloors = true,
			stopAtCeilings = true,
			clampToRoom = true
		}
	}
}
```

Tomb Editor should be able to generate this data from placed emitters and profile UI.

## OCB Compatibility

OCB can remain useful as a compatibility and quick-preset layer, but it should not be the primary long-term data model.

Recommended OCB usage:

```text
OCB 0   - use default editor parameters
OCB 1   - quick preset 1
OCB 2   - quick preset 2
OCB 10+ - legacy compatibility mapping if needed
```

OCB should not carry complex atmosphere settings such as colors, density curves, collision flags, render mode, generated seed, and geometry blocking. Those should live in structured editor parameters and exported Flow data.

## Editor Data Model

Tomb Editor should store object parameters as structured metadata instead of treating everything as a single numeric OCB field.

Recommended object metadata:

```text
objectName
objectType
profileId
presetName
scope
radius
height
density
speed
direction
turbulence
verticalDrift
minSize
maxSize
lifetime
fadeDistance
alpha
renderMode
textureName
generatedDetail
generatedSoftness
generatedVariation
generatedSeed
collideWithGeometry
stopAtWalls
stopAtFloors
stopAtCeilings
clampToRoom
inheritWind
colorA
colorB
```

The editor UI can show beginner-friendly grouped fields instead of exposing every internal field at once.

## Suggested UI Groups

Recommended UI groups for an atmosphere emitter:

```text
Preset
- Effect type
- Preset name
- Generated / Sprite / Existing Effect / Custom

Placement
- Scope
- Anchor name
- Radius
- Height

Appearance
- Density
- Alpha
- Color A
- Color B
- Size range
- Generated detail
- Generated softness
- Generated variation

Movement
- Direction
- Speed
- Turbulence
- Vertical drift
- Inherit global wind

Collision and Room Limits
- Stop at walls
- Stop at floors
- Stop at ceilings
- Clamp to room
- Use geometry collision

Advanced
- Generated seed
- Optional texture name
- Lifetime
- Fade distance
```

The default visible UI should stay simple. Advanced fields can be collapsed.

## Generated Effects First

The default render mode should be generated.

```text
AtmosphereEffectRenderMode.Generated
```

This means the engine creates the visible fog, leaf, ash, dust, or storm layer internally from profile data. Builders should not need to provide sprite assets for common atmosphere effects.

Optional sprite or texture usage should be an explicit choice:

```text
AtmosphereEffectRenderMode.Sprite
```

This is useful for custom builder art, but should not be required for normal atmosphere effects.

## Existing Effects

Existing TombEngine effects should not be duplicated blindly.

For example, if TombEngine already has a firefly-style effect, the atmosphere system should later support an explicit bridge instead of creating a second incompatible firefly implementation.

Recommended bridge direction:

```text
AtmosphereEffectRenderMode.ExistingEffect
presetName = "fireflies"
```

This keeps the atmosphere system open while preserving existing engine behavior where it is already useful.

## Geometry Blocking

The editor must make it clear that normal atmosphere effects should respect level geometry.

Safe default flags:

```text
collideWithGeometry = true
stopAtWalls = true
stopAtFloors = true
stopAtCeilings = true
clampToRoom = true
```

Effects such as leaves, ash, mist, dust, and fog should not visibly pass through walls, floors, or ceilings in normal use.

Special effects can later allow relaxed behavior, but this should be an advanced setting.

## PR Positioning

For the TombEngine PR, the first step should stay conservative:

- Add Flow data shape.
- Add generated atmosphere effect profile data.
- Keep legacy weather fields working.
- Do not force new renderer behavior yet.
- Document that Tomb Editor should later expose this through structured object parameters rather than raw OCB scripting.

For the later Tomb Editor PR, the main feature should be an editor-side `Object Parameters` / `Atmosphere Emitter` workflow that generates the correct Flow data automatically.
