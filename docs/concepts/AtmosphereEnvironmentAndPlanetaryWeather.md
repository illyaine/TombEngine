# Atmosphere Environment Modes and Planetary Weather

## Goal

The atmosphere system should not treat all levels as normal Earth outdoor levels.

A level can be:

```text
- Earth-like outdoor level
- underground / indoor level
- underwater / flooded level
- space / vacuum level
- alien planet
- toxic planet
- artificial dome / habitat
- fantasy atmosphere
```

The system should therefore have an environment mode above weather. Weather should be validated against the current environment.

## Core Idea

Weather is not always physically valid.

Examples:

```text
Space vacuum:
    normal rain should be disabled
    normal snow should be disabled
    normal wind should be disabled or treated as artificial/local

Alien planet:
    rain can exist, but may be acid, toxic, methane, ash, spores, plasma, etc.

Toxic atmosphere:
    rain can damage Lara or objects later
    fog can be poisonous
    particles can be hazardous

Artificial dome:
    artificial rain or snow can exist even in space
    wind can be ventilation-driven
```

This avoids impossible combinations while still allowing creative sci-fi and fantasy setups.

## Proposed Environment Modes

Suggested future enum:

```text
AtmosphereEnvironmentMode
    EarthLike
    Indoor
    Underground
    Underwater
    SpaceVacuum
    AlienPlanet
    ToxicPlanet
    ArtificialDome
    Fantasy
    Custom
```

## Weather Policy

Each environment mode should define a weather policy:

```text
allowNormalRain
allowNormalSnow
allowWind
allowThunder
allowAurora
allowCelestialBodies
allowSpaceLayers
allowToxicWeather
allowArtificialWeather
```

Example default policy:

```text
EarthLike:
    rain yes
    snow yes
    wind yes
    thunder yes
    aurora yes
    celestial yes
    space layers optional
    toxic weather no by default

SpaceVacuum:
    rain no
    snow no
    normal wind no
    thunder no
    aurora optional/fantasy
    celestial yes
    space layers yes
    artificial weather only if explicitly enabled

AlienPlanet:
    rain yes, but can be non-water
    snow yes, but can be ash/crystal/methane/etc.
    wind yes
    toxic weather optional
    celestial yes
    space layers yes

ToxicPlanet:
    rain yes, toxic/acid by default
    fog yes, toxic by default
    wind yes
    thunder optional
    celestial yes
    space layers yes
```

## Planetary Weather Materials

Weather should eventually have a material/content type, not just visual type.

Suggested future enum:

```text
AtmospherePrecipitationMaterial
    Water
    Snow
    Ash
    Sand
    Acid
    ToxicFluid
    Methane
    Spores
    CrystalDust
    RadiationDust
    Plasma
    MagicEnergy
    Custom
```

This allows:

```text
- acid rain
- toxic rain
- methane snow
- ash storms
- crystal dust
- radiation dust
- alien spores
- magic energy rain
```

## Gameplay Hooks

Planetary weather can later affect gameplay through optional flags:

```text
damagePerSecond
surfaceReaction
visibilityPenalty
slipperySurfaces
corrodesMetal
hurtsLara
hurtsEnemies
requiresSuit
safeInWaterRooms
safeInsideDome
```

Important: gameplay impact must be explicit and configurable. Visual toxic rain should not automatically damage gameplay unless configured.

## Space Mode Rules

When `SpaceVacuum` is active:

```text
- normal rain should not render unless artificialWeather is enabled
- normal snow should not render unless artificialWeather is enabled
- normal wind should not affect global weather unless artificialWeather or forceWind is enabled
- celestial and space layers should remain active
- asteroid/debris/comet layers should remain active
- local VFX may still exist, such as leaking gas, sparks, dust, or station interior effects
```

This permits a space level with:

```text
- Earth visible outside
- debris field
- distant station
- no normal rain/snow
- local leaking steam or gas inside a station
```

## Alien Planet Rules

When `AlienPlanet` or `ToxicPlanet` is active:

```text
- precipitation is allowed
- precipitation material should define behavior
- fog can be colored/toxic
- wind can carry dust/spores/ash
- sky can show multiple planets/moons
- weather can be visually hostile without automatically causing damage
```

Example:

```lua
level.atmosphere.environment = Flow.AtmosphereEnvironmentProfile {
    mode = AtmosphereEnvironmentMode.ToxicPlanet,
    artificialWeather = false,
    allowToxicWeather = true
}

level.atmosphere.weather = Flow.WeatherProfile {
    type = WeatherType.Rain,
    strength = 0.7,
    precipitationMaterial = AtmospherePrecipitationMaterial.Acid
}
```

The exact Lua API is not implemented yet. This is the intended direction.

## Dome / Habitat Rule

Artificial domes are important for space levels:

```text
Outside: SpaceVacuum
Inside dome: ArtificialDome
```

The system should later support zones:

```text
AtmosphereZone
    mode
    weatherOverride
    fogOverride
    windOverride
    audioOverride
```

This allows:

```text
- no rain outside in space
- artificial rain inside a biosphere dome
- toxic fog in a damaged greenhouse
- normal indoor ambience in station rooms
```

## Relation to Layered Space Sky Stack

Environment mode controls what is physically/logically allowed.

Layer stack controls what is visually drawn.

Example:

```text
SpaceVacuum
    allow space layers yes
    allow celestial bodies yes
    block normal rain/snow

Layer Stack
    Earth layer
    Moon layer
    debris field layer
    distant station layer
```

These systems should be linked, but not merged into one object.

## Runtime Validation Direction

Future runtime validation should produce warnings, not hard crashes:

```text
Warning: Normal rain is disabled in SpaceVacuum environment.
Warning: Snow is disabled in SpaceVacuum environment unless artificialWeather is enabled.
Warning: Toxic precipitation has damage disabled; it is visual only.
```

This is important for builder-friendly diagnostics.

## Tomb Editor Authoring

Tomb Editor should eventually expose this as:

```text
Atmosphere Environment:
    Earth-like
    Space/Vacuum
    Alien Planet
    Toxic Planet
    Artificial Dome
    Custom

Weather Material:
    Water
    Snow
    Acid
    Ash
    Sand
    Spores
    Methane
    Custom

Gameplay Behavior:
    Visual only
    Damaging
    Requires suit
    Reduces visibility
    Slippery surfaces
```

## Migration Rule

Existing `level.weather`, `level.weatherStrength`, and `level.weatherClustering` must remain valid.

For old levels:

```text
environment mode defaults to EarthLike
normal rain/snow behavior stays unchanged
```

Only levels that opt into new atmosphere environment settings should get strict space/planet behavior.
