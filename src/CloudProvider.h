#pragma once

#include "FilestashClient.h"
#include "StateMachine.h"
#include "StateStore.h"

#include <windows.h>
#include <cfapi.h>
#include <string>
#include <vector>

class CloudProvider {
public:
    CloudProvider(
        FilestashClient& client,
        StateStore& store,
        std::wstring sync_root);
    ~CloudProvider();

    void RegisterSyncRoot();
    void UnregisterSyncRoot();
    void Connect();
    void Disconnect();
    void PopulateNamespace(const std::wstring& relative_path);

private:
    static void CALLBACK OnFetchData(
        const CF_CALLBACK_INFO* callback_info,
        const CF_CALLBACK_PARAMETERS* callback_parameters);

    static void CALLBACK OnFetchPlaceholders(
        const CF_CALLBACK_INFO* callback_info,
        const CF_CALLBACK_PARAMETERS* callback_parameters);

    static void CALLBACK OnCancelFetchData(
        const CF_CALLBACK_INFO* callback_info,
        const CF_CALLBACK_PARAMETERS* callback_parameters);

    FileState LoadOrCreateState(const std::wstring& local_path, const std::wstring& remote_path);
    void PersistEvent(const std::wstring& local_path, const Event& event);

    FilestashClient& client_;
    StateStore& store_;
    std::wstring sync_root_;
    CF_CONNECTION_KEY connection_key_{};
    StateMachine state_machine_;
};
