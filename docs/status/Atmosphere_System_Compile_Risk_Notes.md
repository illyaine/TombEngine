# Atmosphere System Compile Risk Notes

## Purpose

This file tracks the known compile-sensitive areas of the current `atmosphere_system` branch before local build verification is available.

No build has been run for this branch in this environment.

## Highest Risk: Flow Enum Registration

The current atmosphere Flow registration uses `parent.new_enum` for these enum groups:

```text
WeatherQuality
AtmosphereEffectType
AtmosphereEffectScope
AtmosphereEffectRenderMode
```

Existing TombEngine Flow code visibly uses read-only tables for classic enum constants, for example `WEATHER_TYPES` is registered through `MakeReadOnlyTable` in `FlowHandler.cpp`.

If local compile fails around `new_enum`, use the existing TombEngine style instead:

```text
1. Keep enum values as C++ enum class types.
2. Add static const unordered_map tables for atmosphere enum values.
3. Register those tables in FlowHandler with MakeReadOnlyTable.
4. Remove the parent.new_enum calls from Atmosphere::Register.
```

Suggested table names:

```text
WEATHER_QUALITIES
ATMOSPHERE_EFFECT_TYPES
ATMOSPHERE_EFFECT_SCOPES
ATMOSPHERE_EFFECT_RENDER_MODES
```

Suggested script names:

```text
WeatherQuality
AtmosphereEffectType
AtmosphereEffectScope
AtmosphereEffectRenderMode
```

## Vector Binding Risk

`Flow.Atmosphere.effects` currently exposes:

```cpp
std::vector<AtmosphereEffectProfile> Effects
```

If Lua assignment from a table does not bind cleanly in this sol2 setup, the fallback is to use the same table-wrapper pattern used by other Flow arrays in TombEngine, or to temporarily keep local effect profiles as generated/exported data only until Tomb Editor produces stable metadata.

## Project File Risk

`TombEngine.vcxproj` currently has a noisy diff. The intended functional change is only that these files are part of the project:

```text
Scripting\Internal\TEN\Flow\Atmosphere\Atmosphere.h
Scripting\Internal\TEN\Flow\Atmosphere\Atmosphere.cpp
```

Before PR, clean the project file locally so the diff does not look like a full project-file rewrite.

## Include Risk

`Atmosphere.h` now explicitly includes:

```cpp
#include <string>
#include <unordered_map>
#include <vector>
```

This avoids relying on transitive includes from `framework.h` or other headers.

## Local Build Check Order

Recommended local check order:

```text
1. Build TombEngine.
2. If enum registration fails, switch atmosphere enums to MakeReadOnlyTable style.
3. If effects vector binding fails, defer the effects array or wrap it in the existing Flow table style.
4. Clean TombEngine.vcxproj noisy diff.
5. Rebuild before opening PR.
```
