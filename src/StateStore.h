#pragma once

#include "StateMachine.h"

#include <optional>
#include <string>
#include <unordered_map>

class StateStore {
public:
    virtual ~StateStore() = default;

    virtual std::optional<FileState> Get(const std::wstring& local_path) = 0;
    virtual void Put(const FileState& state) = 0;
};

class MemoryStateStore final : public StateStore {
public:
    std::optional<FileState> Get(const std::wstring& local_path) override;
    void Put(const FileState& state) override;

private:
    std::unordered_map<std::wstring, FileState> states_;
};
