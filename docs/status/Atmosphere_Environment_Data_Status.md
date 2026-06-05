# Atmosphere Environment Data Status

## Current Step

This branch now contains a prepared environment/planetary weather data header:

```text
TombEngine/Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereEnvironment.h
```

The header is intentionally prepared as an isolated foundation first.

## Added Data Shapes

```text
AtmosphereEnvironmentMode
AtmospherePrecipitationMaterial
AtmosphereEnvironmentProfile
```

## Environment Modes

```text
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

## Precipitation Materials

```text
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

## Prepared Policy Helpers

```text
IsSpaceMode()
IsAlienOrToxicMode()
IsArtificialEnvironment()
AllowsWeatherType()
AllowsNormalPrecipitation()
AllowsWind()
HasHazardousPrecipitation()
HasGameplayHazard()
```

## Current Runtime Status

The header is not yet wired into `Flow.Atmosphere` or registered in Lua.

This is intentional for this step because the atmosphere header and project file already have larger pending changes. The safer order is:

```text
1. Keep environment policy isolated first.
2. Confirm compile behavior of current Atmosphere.h / Atmosphere.cpp changes.
3. Add include and profile member into Flow.Atmosphere.
4. Register environment enums/profile in Atmosphere.cpp.
5. Add environment into AtmosphereRuntimeSnapshot and AtmosphereRenderData.
6. Add runtime warnings or validation later.
```

## Intended Behavior Later

`SpaceVacuum` should block normal rain/snow unless artificial weather is explicitly enabled.

`AlienPlanet` and `ToxicPlanet` should allow non-water precipitation such as acid, methane, spores, ash, or radiation dust.

Gameplay damage must remain optional. Toxic-looking weather should be able to stay visual-only unless configured otherwise.

## Compatibility Rule

Existing levels must default to Earth-like behavior.

```text
Legacy level.weather remains valid.
Legacy rain/snow behavior remains unchanged.
New environment policy only applies when a level opts into it.
```
