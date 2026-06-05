# Atmosphere Moon 3D Render Layer

## Goal

The moon should not look like a flat sprite pasted onto the sky. The default implementation should be an engine-rendered celestial layer with a convincing 3D appearance.

This document defines the intended renderer direction without activating draw code yet.

## Recommended Default

The default moon path should be:

```text
Engine-rendered sky/celestial layer
    + sky pitch/yaw positioning
    + diffuse moon texture
    + optional normal map
    + optional height/parallax influence
    + phase/terminator shading
    + rim/halo lighting
    + fog fading
```

This should behave like a sky object, not like a normal placed level object.

Reasons:

```text
- The moon should stay visually locked to the sky, not to a room or sector.
- It should not need collision, trigger logic, AI logic, or normal object update cost.
- It should remain compatible with wide outdoor rooms and horizon setups.
- Pitch/yaw positioning is easier for builders than world-space coordinates.
- The renderer can make it look spherical without needing a real mesh in the level.
```

## 3D Appearance

The moon should look three-dimensional through shader/data effects:

```text
Diffuse texture         Base moon surface.
Normal texture          Crater/surface lighting detail.
Height/parallax value   Optional relief impression.
Phase                   Full moon, crescent, half moon, etc.
Terminator softness     Soft transition between lit and dark moon side.
Rim light               Subtle edge highlight.
Halo                    Soft glow around the moon.
Moonlight color         Separate color for scene tint / light shafts later.
```

This means the visual result can look like a sphere even when rendered as a sky billboard/quad.

## Lua-Level Positioning

Moon position should remain scriptable through sky angles:

```lua
level.atmosphere.moon = Flow.MoonProfile {
    enabled = true,
    pitch = -18,
    yaw = 225,
    size = 1.2,
    intensity = 0.85,
    phase = 1.0
}
```

This avoids tying the moon to a physical object position.

## Texture Source

The profile should support texture names rather than requiring a normal object slot:

```text
textureName        diffuse moon texture
normalTextureName  optional normal map
heightTextureName  optional height/parallax texture
```

The exact asset lookup can be decided later by the renderer integration:

```text
- built-in default moon texture if no textureName is set
- project texture name if provided
- later Tomb Editor UI picker for moon texture slots
```

## Object Slot Alternative

A real object/mesh moon should be optional only:

```text
MoonRenderMode.SkyBillboard      default, shader-generated 3D illusion
MoonRenderMode.SkySphere         optional future mesh/sphere sky layer
MoonRenderMode.ObjectSlot        optional future special object slot
```

The object-slot path should not be the default because it would create unnecessary object-management requirements for most builders.

Possible use cases for an object-slot mode:

```text
- stylized moon mesh
- animated / broken moon
- planet or artificial moon object
- custom geometry with special material
```

But even then, it should still be rendered as a sky/celestial object and not behave like a normal level object.

## Light Shafts Coupling

Moon-driven light shafts should be able to inherit the moon direction:

```lua
Flow.LightShaftProfile {
    enabled = true,
    scope = AtmosphereEffectScope.Global,
    inheritMoonDirection = true
}
```

For local/nullmesh shafts, the nullmesh should define the source or window opening, while the moon can still provide direction/color if requested later.

## Renderer Hook Direction

The renderer should use the existing non-rendering data path:

```cpp
AtmosphereRenderData renderData = level.CreateAtmosphereRenderData();
AtmosphereRenderPlan renderPlan = level.CreateAtmosphereRenderPlan();
```

The moon render layer should only activate when:

```text
renderPlan.DrawMoon == true
```

No renderer should read Flow/Lua directly.

## Suggested Future Fields

The current `MoonProfile` already has the basic fields. Later extension should add, if needed:

```text
normalTextureName
heightTextureName
surfaceRelief
terminatorSoftness
rimLightIntensity
parallaxStrength
rotation
renderMode
objectSlotId / objectId only for optional object-slot mode
```

These should be added only when the first renderer pass needs them, to avoid growing the Flow API faster than runtime support.

## Compatibility Rule

Legacy sky, horizon, starfield, and lens flare must remain valid. The moon layer should add a new path, not replace existing sky features in the first implementation pass.
