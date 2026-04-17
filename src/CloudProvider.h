#pragma once

#include "FilestashClient.h"
#include "StateMachine.h"
#include "StateStore.h"

#include <windows.h>
#include <cfapi.h>
#include <memory>
#include <string>

class CloudProvider {
public:
    CloudProvider(
        std::shared_ptr<FilestashClient> client,
        std::shared_ptr<StateStore> store,
        std::wstring sync_root);
    ~CloudProvider();

    void RegisterSyncRoot();
    void Connect();
    void Disconnect();

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

    std::shared_ptr<FilestashClient> client_;
    std::shared_ptr<StateStore> store_;
    std::wstring sync_root_;
    CF_CONNECTION_KEY connection_key_{};
    StateMachine state_machine_;
};
