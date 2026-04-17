#include "StateMachine.h"

FileState StateMachine::Apply(const FileState& current, const Event& event) const {
    FileState next = current;
    next.last_error.clear();

    switch (event.type) {
    case EventType::HydrationRequested:
        next.materialization_state = MaterializationState::Hydrating;
        break;
    case EventType::HydrationSucceeded:
        next.materialization_state = MaterializationState::Hydrated;
        next.sync_state = SyncState::Clean;
        if (!event.modified_at.empty()) {
            next.base_modified_at = event.modified_at;
        }
        break;
    case EventType::HydrationFailed:
        next.materialization_state = MaterializationState::Placeholder;
        next.sync_state = SyncState::Error;
        next.last_error = event.message;
        break;
    case EventType::LocalModified:
        next.materialization_state = MaterializationState::Hydrated;
        next.sync_state = SyncState::Dirty;
        break;
    case EventType::UploadRequested:
        next.sync_state = SyncState::Uploading;
        break;
    case EventType::UploadSucceeded:
        next.sync_state = SyncState::Clean;
        if (!event.modified_at.empty()) {
            next.base_modified_at = event.modified_at;
        }
        break;
    case EventType::UploadConflict:
        next.sync_state = SyncState::Conflict;
        next.last_error = event.message;
        break;
    case EventType::UploadFailed:
        next.sync_state = SyncState::Error;
        next.last_error = event.message;
        break;
    case EventType::RemoteInvalidated:
        if (next.sync_state == SyncState::Clean) {
            next.materialization_state = MaterializationState::Placeholder;
        } else {
            next.sync_state = SyncState::Conflict;
            next.last_error = L"Remote content changed while local state was not clean";
        }
        break;
    case EventType::Reset:
        next.materialization_state = MaterializationState::Placeholder;
        next.sync_state = SyncState::Clean;
        next.base_modified_at.clear();
        next.last_error.clear();
        break;
    }

    return next;
}
