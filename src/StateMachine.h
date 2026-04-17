#pragma once

#include <optional>
#include <string>

enum class MaterializationState {
    Placeholder,
    Hydrating,
    Hydrated,
};

enum class SyncState {
    Clean,
    Dirty,
    Uploading,
    Conflict,
    Error,
};

enum class EventType {
    HydrationRequested,
    HydrationSucceeded,
    HydrationFailed,
    LocalModified,
    UploadRequested,
    UploadSucceeded,
    UploadConflict,
    UploadFailed,
    RemoteInvalidated,
    Reset,
};

struct FileState {
    std::wstring local_path;
    std::wstring remote_path;
    std::wstring base_modified_at;
    MaterializationState materialization_state = MaterializationState::Placeholder;
    SyncState sync_state = SyncState::Clean;
    std::wstring last_error;
};

struct Event {
    EventType type;
    std::wstring modified_at;
    std::wstring message;
};

class StateMachine {
public:
    FileState Apply(const FileState& current, const Event& event) const;
};
