# Atmosphere Space and Celestial Layer

## Goal

The atmosphere system should not be limited to a single moon. It should support a broader sky/celestial layer for outdoor, fantasy, sci-fi, and space levels.

The generic layer should support:

```text
- moons
- planets
- Earth-like distant bodies
- large stars / suns
- fantasy celestial bodies
- comets
- asteroid belts
- meteor streams
- distant space debris
- derelict ships / stations as distant sky objects
- artificial orbiting structures
```

This document defines the intended data and renderer direction without activating draw code yet.

## Naming Direction

Use generic terminology instead of moon-specific terminology:

```text
CelestialBodyProfile
SpaceObjectProfile
SpaceDebrisProfile
AtmosphereCelestialLayer
```

The current `MoonProfile` should be treated as an early/specialized stepping stone, not the final public shape for all sky bodies.

## Recommended High-Level Model

The future atmosphere sky model should split celestial content into three groups:

```text
Celestial bodies
    large, mostly fixed sky bodies: moons, planets, stars, suns

Space objects
    distinct visible distant objects: station, derelict ship, artificial satellite, large comet

Space fields
    many small generated elements: asteroid belt, debris field, meteor stream, dust belt
```

This avoids forcing all space visuals into one overloaded object type.

## Celestial Bodies

Celestial bodies should be rendered by the engine as sky/celestial layers, not normal gameplay objects by default.

Examples:

```lua
level.atmosphere.celestialBodies = {
    Flow.CelestialBodyProfile {
        enabled = true,
        name = "Earth",
        type = CelestialBodyType.Planet,
        pitch = -12,
        yaw = 210,
        size = 1.8,
        intensity = 0.9,
        phase = 1.0,
        textureName = "earth_diffuse",
        normalTextureName = "earth_normal",
        heightTextureName = "earth_height"
    },

    Flow.CelestialBodyProfile {
        enabled = true,
        name = "SmallMoon",
        type = CelestialBodyType.Moon,
        pitch = -24,
        yaw = 246,
        size = 0.55,
        intensity = 0.6,
        phase = 0.75,
        textureName = "moon_diffuse"
    }
}
```

## 3D Appearance Requirement

A planet or moon must not look like a flat sticker.

Recommended visual model:

```text
Sky billboard / sky dome patch
    + diffuse texture
    + optional normal map
    + optional height/parallax map
    + spherical lighting approximation
    + phase / terminator shading
    + rim lighting
    + atmosphere/halo glow
    + optional slow rotation
```

This creates a 3D-looking planet without requiring a real world-space mesh.

## Large Stars and Suns

Large stars/suns should be supported as celestial bodies with star-specific parameters:

```text
coronaIntensity
flareIntensity
surfaceNoiseStrength
surfaceAnimationSpeed
lightColor
lightShaftInfluence
```

They can share the same base celestial-body layer but use a different render mode or type.

Example use cases:

```text
- sci-fi planet orbiting a blue star
- fantasy red sun
- distant binary stars
- huge sun with heat haze / corona
```

## Comets

Comets should be treated as distant celestial/space objects, with a body plus tail.

Suggested future fields:

```text
name
enabled
pitch
yaw
size
intensity
textureName
tailLength
tailWidth
tailIntensity
tailColor
driftSpeed
driftDirection
sortOrder
```

A comet may be static for atmosphere composition or slowly animated across the sky.

## Asteroids, Debris, and Space Junk

Space debris should not be implemented as hundreds of normal objects by default.

Recommended paths:

```text
SpaceDebrisProfile / SpaceFieldProfile
    generated field of small sky-space elements

SpaceObjectProfile
    individual larger object such as derelict ship or station
```

Suggested `SpaceFieldProfile` fields:

```text
enabled
name
type = AsteroidBelt / DebrisField / MeteorStream / DustCloud
pitch
yaw
spreadX
spreadY
density
minSize
maxSize
depthVariation
speed
driftDirection
rotationVariation
textureSetName
color
alpha
sortOrder
```

This allows space levels to have:

```text
- asteroid belts
- broken satellite fragments
- drifting metal debris
- meteor streams
- distant dust clouds
```

without normal object overhead.

## Distant Space Objects

Larger space structures may need a mesh-like representation, but still as sky-space objects:

```text
SpaceObjectProfile
    name
    objectId or meshReference
    pitch/yaw
    size
    roll
    rotationSpeed
    intensity
    fogFade
    sortOrder
```

Possible examples:

```text
- derelict ship silhouette
- orbital station
- broken ring structure
- huge temple fragment in space
- artificial satellite
```

These should be rendered as far sky/celestial objects, not normal gameplay objects, unless a builder intentionally places a real reachable object.

## Object Slot Policy

Normal object slots should be optional, not required.

Default:

```text
Engine sky/celestial render data
```

Optional special mode:

```text
ObjectSlot / MeshReference for large custom sky-space models
```

Reasons:

```text
- Space bodies should stay visually fixed in the sky.
- They do not need normal collision or AI.
- They should not disappear due to room/object culling rules.
- Multiple distant objects should stay cheap.
```

## Render Planning

The current `AtmosphereRenderPlan` is the first docking point.

Future render plan should split this more clearly:

```text
DrawWeather
DrawCelestialBodies
DrawSpaceObjects
DrawSpaceFields
DrawAurora
DrawGlobalLightShafts
DrawLocalLightShafts
DrawGlobalEffects
DrawLocalEffects
```

This allows TEN to skip whole render passes when a level does not use space/celestial content.

## Light Shaft Coupling

Light shafts should be able to inherit direction/color from named celestial bodies:

```lua
Flow.LightShaftProfile {
    enabled = true,
    scope = AtmosphereEffectScope.Global,
    sourceBodyName = "BlueStar",
    inheritCelestialBodyDirection = true
}
```

The current moon-specific direction concept should become generic later:

```text
inheritCelestialBodyDirection
sourceBodyName
sourceBodyType
```

## Migration From MoonProfile

Current state:

```text
MoonProfile exists as a narrow early data path.
```

Recommended migration:

```text
1. Keep MoonProfile temporarily for simple moon tests.
2. Add CelestialBodyProfile as the generic multi-body path.
3. Internally map MoonProfile to one CelestialBodyProfile if needed.
4. Prefer level.atmosphere.celestialBodies for new complex levels.
5. Avoid expanding MoonProfile too much; put future work into CelestialBodyProfile.
```

## Tomb Editor Authoring

Tomb Editor should eventually provide an atmosphere/celestial editor UI:

```text
- Add planet / moon / star
- Pick texture set
- Pick normal / height texture
- Set pitch/yaw visually
- Set size and intensity
- Set phase and terminator softness
- Add asteroid/debris field
- Add comet
- Preview sky composition
```

For space debris fields, the editor should expose preset-based controls rather than asking builders to hand-write dense Lua tables.

## Compatibility Rule

Legacy sky, horizon, starfield, lens flare, and the current atmosphere data must continue to work.

The new celestial/space layer should add a modern path beside the legacy sky path. It should not break existing levels.
