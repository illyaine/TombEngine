# Atmosphere System Compile Risk Notes

## Purpose

This file tracks the known compile-sensitive areas of the current `atmosphere_system` branch before local build verification is available.

No build has been run for this branch in this environment.

## Flow Enum Registration

The atmosphere Flow enum groups are currently exposed as normal Flow tables from static lookup maps:

```text
WeatherQuality
AtmosphereEffectType
AtmosphereEffectScope
AtmosphereEffectRenderMode
```

The earlier `parent.new_enum` path was removed to reduce sol2 compatibility risk.

Current table names:

```text
WEATHER_QUALITIES
ATMOSPHERE_EFFECT_TYPES
ATMOSPHERE_EFFECT_SCOPES
ATMOSPHERE_EFFECT_RENDER_MODES
```

Current script names:

```text
WeatherQuality
AtmosphereEffectType
AtmosphereEffectScope
AtmosphereEffectRenderMode
```

Remaining possible risk: these tables are registered inside `Atmosphere::Register` via `parent.set(...)`, not through `FlowHandler::_handler.MakeReadOnlyTable(...)`. This keeps the change localized and avoids a broad `FlowHandler.cpp` edit, but the values may not be read-only like classic `WeatherType`.

If maintainers prefer the existing strict pattern, move registration into `FlowHandler.cpp` later with `MakeReadOnlyTable`.

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
2. If table enum registration fails, move atmosphere enum tables to FlowHandler MakeReadOnlyTable registration.
3. If effects vector binding fails, defer the effects array or wrap it in the existing Flow table style.
4. Clean TombEngine.vcxproj noisy diff.
5. Rebuild before opening PR.
```
