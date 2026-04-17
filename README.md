# filestash-cfapi

Fresh Windows Cloud Files API client for Filestash.

This project intentionally starts from a state-machine-first design instead of
deriving sync behavior directly from `CfAPI` callbacks.

## Goals

- Project a Filestash namespace as a Windows Cloud Files sync root
- Keep metadata projection and content hydration separate from sync policy
- Track per-file state durably instead of relying on callback order
- Make room for conflict detection and recovery before uploads are implemented
- Treat modification time as the current backend version token until something stronger exists

## Project Layout

- `src/CloudProvider.h`: `CfAPI` integration surface
- `src/StateMachine.h`: sync state definitions and transitions
- `src/StateStore.h`: durable client state abstraction
- `src/FilestashClient.h`: backend API abstraction
- `docs/architecture.md`: initial design notes

## Status

This is a scaffold:

- callback wiring exists
- state machine and store interfaces exist
- hydration/upload behavior is still placeholder logic
- SQLite integration is not implemented yet

## Build

Use a Windows toolchain with the Windows SDK installed:

```bash
cmake -S . -B build -A x64
cmake --build build --config Debug
```
