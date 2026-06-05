# Object Parameter Runtime System

Branch: `object_parameter_system`

## Goal

This branch prepares a neutral runtime foundation for a future Object Parameter System in TEN.
The system is intended to become the runtime-side replacement path for legacy OCB-driven object configuration.
OCB is not migrated or changed in this branch.

## Scope of this step

Implemented as a small, non-activating runtime base:

- neutral object parameter value container
- neutral object reference container
- parameter entries keyed by provider, definition set, object reference and parameter id
- lookup service for provider/object/parameter queries
- consumer registration hook filtered by provider id
- explicit dispatch entry/provider methods for future systems

No atmosphere-specific logic was added.
No object behavior is changed.
No level loading, PRJ2 loading, Lua loading or OCB migration is activated.

## Runtime model

The current runtime-side model is intentionally generic:

- `providerId`
- `definitionSetId`
- object reference:
  - item index
  - script name
  - Lua/script reference
  - object id
- `parameterId`
- typed value:
  - none
  - boolean
  - integer
  - float
  - string

Future systems can register a consumer callback for their own provider id and process only their own entries.
Example future consumers include sound, particles, light helpers, trap/puzzle helpers, AI helpers, camera helpers and atmosphere handlers.

## Files added

- `TombEngine/Game/ObjectParameters/ObjectParameterRegistry.h`

## Integration notes

This first step does not yet define the binary/export format used by Tomb Editor.
That must be aligned with the Tomb Editor `object_parameter_system` branch once PRJ2 save/load and export format are finalized.

Expected next TEN steps:

1. Decide the serialized runtime chunk or script-side export format.
2. Add a loader/importer that fills `TEN::ObjectParameters::GetObjectParameterRegistry()` during level initialization.
3. Expose read-only lookup helpers to Lua only if needed.
4. Add real consumers one by one, each filtered by its own provider id.
5. Keep OCB handling as legacy fallback until equivalent object-parameter definitions exist.

## Deliberate non-goals in this branch

- no atmosphere implementation
- no weather implementation
- no OCB migration
- no object behavior changes
- no automatic dispatch during gameplay loop
- no hard dependency on Tomb Editor internals
- no large provider framework

## Build/test status

Build was not run in this environment.
Local build and gameplay test must be performed separately.
