# Architecture

## Core principle

`CfAPI` is treated as an event source and projection mechanism, not as the
source of truth for synchronization state.

## Sources of truth

- Filestash backend: remote namespace and remote modification time state
- Local state store: durable client-side sync state
- Windows `CfAPI` and file notifications: signals that feed the state machine

## Initial file model

Each tracked file should carry:

- local path
- remote path
- remote id when available
- base modification time token
- hydration state
- sync state
- last error

## Initial states

- `Placeholder`
- `Hydrating`
- `Clean`
- `Dirty`
- `Uploading`
- `Conflict`
- `Error`

## Initial event flow

1. A `CfAPI` callback arrives.
2. The callback is normalized into an internal event.
3. The event is applied to the per-file state machine.
4. The state store persists the new state.
5. Any resulting work is scheduled outside the callback.

## Why this shape

The previous POC handled hydration and validity checks directly inside
callbacks. That is enough for namespace projection, but not enough for:

- crash recovery
- retry semantics
- local dirty tracking
- upload preconditions
- conflict detection
