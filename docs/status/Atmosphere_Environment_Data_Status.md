# Atmosphere Environment Data Status

## Current Step

This branch now contains the first wired environment/planetary weather data path:

```text
TombEngine/Scripting/Internal/TEN/Flow/Atmosphere/AtmosphereEnvironment.h
TombEngine/Scripting/Internal/TEN/Flow/Level/FlowLevel.h
TombEngine/Scripting/Internal/TEN/Flow/Level/FlowLevel.cpp
```

The profile is still intentionally lightweight. It provides policy data and read-path filtering only. It does not render new effects by itself and does not add gameplay damage handling yet.

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

`AtmosphereEnvironmentProfile` is now registered for Lua through `Flow.Level` registration.

```text
Flow.AtmosphereEnvironmentMode
Flow.AtmospherePrecipitationMaterial
Flow.AtmosphereEnvironmentProfile
Flow.Level.atmosphereEnvironment
```

`Flow.Level` now owns an optional `AtmosphereEnvironmentProfile` named `atmosphereEnvironment`.

The existing weather read path now applies this profile before returning weather type, weather strength, weather clustering, runtime snapshot data, and render data.

The current behavior is intentionally conservative:

```text
1. Existing levels keep legacy weather behavior because atmosphereEnvironment.enabled defaults to false.
2. Restricted modes such as SpaceVacuum, Indoor, Underground, and Underwater can block normal precipitation.
3. Artificial weather can explicitly allow weather in restricted environments.
4. Hazardous precipitation is blocked unless allowToxicWeather or forceAllowPrecipitation is enabled.
5. SpaceVacuum blocks wind unless artificialWeather or forceAllowWind is enabled.
6. Gameplay damage remains data-only and is not applied by this step.
```

## Intended Behavior Later

`SpaceVacuum` should block normal rain/snow unless artificial weather is explicitly enabled.

`AlienPlanet` and `ToxicPlanet` should allow non-water precipitation such as acid, methane, spores, ash, or radiation dust.

Gameplay damage must remain optional. Toxic-looking weather should be able to stay visual-only unless configured otherwise.

Future steps can decide whether the environment profile should stay on `Flow.Level` or move into `Flow.Atmosphere` once the renderer and editor boundary are ready for deeper integration.

## Compatibility Rule

Existing levels must default to Earth-like behavior.

```text
Legacy level.weather remains valid.
Legacy rain/snow behavior remains unchanged.
New environment policy only applies when a level opts into it.
```
