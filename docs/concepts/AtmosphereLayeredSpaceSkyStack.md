# Atmosphere Layered Space Sky Stack

## Goal

Space and celestial visuals should be built as a stack of controllable layers, not as one monolithic moon/planet object.

This is important for space levels, fantasy skies, large planets, multiple moons, asteroid fields, comets, debris, stars, and distant artificial structures.

## Core Rule

Use multiple independent sky-space layers:

```text
Layer 00  Deep background / nebula / space gradient
Layer 10  Distant starfield / large stars
Layer 20  Far planets / large celestial bodies
Layer 30  Moons / smaller orbiting bodies
Layer 40  Comets / moving celestial objects
Layer 50  Asteroid belts / debris fields
Layer 60  Distant artificial objects / stations / derelicts
Layer 70  Aurora / energy sky effects
Layer 80  Global light shafts / god rays
Layer 90  Weather / atmosphere overlays
```

Layer numbers are conceptual. The engine can use a small enum or sort order later.

## Why Layers Are Needed

A single celestial object list is not enough because builders need separate control over:

```text
- render order
- depth impression
- opacity
- fog fade
- movement speed
- parallax amount
- rotation
- blend mode
- visibility by room/level state
- quality/performance budgets
- texture sets
- editor grouping
```

Example: a space level might need:

```text
- background nebula layer
- dense star layer
- large Earth-like planet layer
- two moon layers
- slow comet layer
- asteroid belt layer
- drifting debris layer
- distant station layer
```

All of those should be adjustable separately.

## Proposed Layer Data Model

Future generic layer base:

```text
SkySpaceLayerProfile
    enabled
    name
    type
    sortOrder
    opacity
    blendMode
    pitch
    yaw
    roll
    size
    spreadX
    spreadY
    depth
    parallax
    speed
    direction
    rotationSpeed
    fadeWithFog
    quality
```

Specialized payloads can then exist per layer type.

## Suggested Layer Types

```text
BackgroundGradient
Nebula
Starfield
LargeStar
CelestialBody
Planet
Moon
Comet
AsteroidField
DebrisField
MeteorStream
DistantObject
DistantStation
EnergyField
Aurora
LightShafts
WeatherOverlay
Custom
```

## Celestial Body Layer

For planets, moons, suns, and fantasy sky bodies:

```text
CelestialBodyLayer
    enabled
    name
    type = Planet / Moon / Star / FantasyBody
    sortOrder
    pitch
    yaw
    size
    intensity
    phase
    terminatorSoftness
    rimLightIntensity
    haloIntensity
    textureName
    normalTextureName
    heightTextureName
    surfaceRelief
    rotation
    rotationSpeed
    lightColor
```

This allows multiple planets and moons:

```lua
level.atmosphere.celestialLayers = {
    Flow.CelestialBodyLayer {
        name = "Earth",
        type = CelestialBodyType.Planet,
        sortOrder = 20,
        pitch = -10,
        yaw = 210,
        size = 1.8,
        textureName = "earth_diffuse"
    },

    Flow.CelestialBodyLayer {
        name = "MoonA",
        type = CelestialBodyType.Moon,
        sortOrder = 30,
        pitch = -24,
        yaw = 240,
        size = 0.55,
        textureName = "moon_a"
    },

    Flow.CelestialBodyLayer {
        name = "MoonB",
        type = CelestialBodyType.Moon,
        sortOrder = 31,
        pitch = -16,
        yaw = 265,
        size = 0.35,
        textureName = "moon_b"
    }
}
```

## Space Field Layers

Asteroids, debris, dust, and meteor streams should be generated fields, not hundreds of gameplay objects.

```text
SpaceFieldLayer
    enabled
    name
    type = AsteroidField / DebrisField / MeteorStream / DustCloud
    sortOrder
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
```

This gives control over multiple independent fields:

```text
- far asteroid belt
- close slow debris
- fast meteor stream
- small dust cloud
```

Each field can have its own speed, density, and sort order.

## Distant Object Layers

Large distant ships, space stations, temples, and artificial structures should use a separate object-like sky layer.

```text
DistantObjectLayer
    enabled
    name
    sortOrder
    meshName / objectId / modelReference
    pitch
    yaw
    roll
    size
    rotationSpeed
    intensity
    fadeWithFog
    parallax
```

This should still be sky-space rendering, not normal room object rendering by default.

## Blend Modes

Useful blend modes:

```text
Opaque
AlphaBlend
Additive
SoftAdditive
Multiply
Screen
Energy
```

Examples:

```text
Planets           AlphaBlend / Opaque-like sky billboard
Nebula            Additive / SoftAdditive
Comet tail        SoftAdditive
Debris            AlphaBlend
Energy field      Additive
Light shafts      SoftAdditive
```

## Parallax and Depth

Layers need controlled depth impression:

```text
parallax = 0.0    fixed infinite sky
parallax = 0.1    very distant movement
parallax = 0.3    closer debris field
parallax = 0.5    strong close sky-space layer
```

Do not use true level-space coordinates for normal celestial content. Use sky-space depth/parallax unless the builder intentionally places a real gameplay object.

## Editor Requirements

Tomb Editor should later expose this as a layer stack UI:

```text
[+] Add Layer
    Background / Starfield / Planet / Moon / Comet / Asteroid Field / Debris / Station / Aurora / Light Shaft

Layer list:
    visibility toggle
    name
    type icon
    sort order
    opacity
    lock
    duplicate
    delete
```

Each layer should have type-specific controls.

## Runtime Render Plan

The current `AtmosphereRenderPlan` should later become more granular:

```text
DrawBackgroundLayers
DrawStarLayers
DrawCelestialBodyLayers
DrawSpaceFieldLayers
DrawDistantObjectLayers
DrawAurora
DrawLightShafts
DrawWeather
```

The engine should skip unused passes completely.

## Migration Rule

Current `MoonProfile` should not become the final all-purpose system.

Preferred migration:

```text
1. Keep MoonProfile for simple compatibility/testing.
2. Add generic layer-stack types later.
3. Treat MoonProfile as a shortcut for one CelestialBodyLayer.
4. Use the layer stack for new advanced space/fantasy skies.
```

## Compatibility Rule

Legacy sky fields must remain valid:

```text
layer1
layer2
horizon1
horizon2
starfield
lensFlare
weather
storm
```

The layer stack should be additive and should not break existing levels.
