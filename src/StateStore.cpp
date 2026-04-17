#include "StateStore.h"

std::optional<FileState> MemoryStateStore::Get(const std::wstring& local_path) {
    const auto it = states_.find(local_path);
    if (it == states_.end()) {
        return std::nullopt;
    }

    return it->second;
}

void MemoryStateStore::Put(const FileState& state) {
    states_[state.local_path] = state;
}
