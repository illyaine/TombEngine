# Atmosphere Celestial Render Activation

## Purpose

This document records the current non-drawing render activation layer for celestial atmosphere objects.

The branch does not yet render planets, moons, comets, debris, galaxy bands, or generated space layers. It now prepares the data and activation decisions that a later renderer can consume without reading Lua or Flow structures directly.

## Flow Data

Celestial objects are authored through:

```lua
level.atmosphereCelestial = Flow.AtmosphereCelestialProfile {
    enabled = true,
    bodies = {
        Flow.AtmosphereCelestialBodyProfile {
            enabled = true,
            type = Flow.AtmosphereCelestialBodyType.Planet,
            renderMode = Flow.AtmosphereCelestialRenderMode.Sphere3D,
            name = "earth",
            textureName = "earth_day",
            pitch = -12,
            yaw = 210,
            size = 1.4,
            intensity = 1.0
        }
    }
}
```

This is intended as a script-level foundation only. Tomb Editor should later provide presets and validation so normal builders do not have to write large celestial body arrays manually.

## Runtime Data Path

The current branch prepares:

```cpp
AtmosphereCelestialRenderData celestialData = level.CreateAtmosphereCelestialRenderData();
SkyAtmosphereRenderData skyData = level.CreateSkyAtmosphereRenderData();
```

`SkyAtmosphereRenderData` now carries both the existing atmosphere render plan and the celestial render activation data.

## Activation Categories

The activation data classifies enabled bodies into renderer-relevant buckets:

```text
BodyCount
SpaceBodyCount
Sphere3DBodyCount
BillboardBodyCount
HorizonObjectBodyCount
GeneratedLayerCount
LightSourceCount
HideLegacyMoon
HideStarfield
```

This gives the later renderer a clean decision layer:

```text
Sphere3DBodyCount > 0        -> future 3D planet/moon sphere pass
BillboardBodyCount > 0       -> future flat sky-object pass
HorizonObjectBodyCount > 0   -> future horizon-object bridge pass
GeneratedLayerCount > 0      -> future generated galaxy/nebula layer pass
LightSourceCount > 0         -> future light direction/color bridge
```

## What Is Not Active Yet

No visible render code is active yet:

```text
- No shader changes.
- No horizon mesh hacks.
- No Aurora layer draw calls.
- No 3D planet sphere draw calls.
- No billboard sky body draw calls.
- No generated galaxy/nebula layer draw calls.
- No legacy starfield suppression is forced in renderer code yet.
```

This is deliberate. The next renderer step should be small and visible, preferably one controlled sky-object layer after compile is confirmed.

## Recommended Renderer Sequence

Recommended order after local compile confirmation:

```text
1. Build-confirm Flow and activation data.
2. Add a minimal renderer-side read of SkyAtmosphereRenderData.
3. Add a debug/non-visual log or breakpoint-only activation check, not user-visible text.
4. Add one simple billboard sky object path.
5. Add Sphere3D body path for moon/planet rendering.
6. Add generated galaxy/nebula layer after object rendering is stable.
7. Add Aurora as a separate layer, not as a Horizon/Sky.hlsl hack.
```

## Design Rule

Celestial rendering must remain a proper atmosphere layer stack. It should not be implemented by modifying legacy sky layer shaders in a way that makes Aurora, planets, stars, and galaxy layers fight each other.
