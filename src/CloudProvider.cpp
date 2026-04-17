#include "CloudProvider.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

namespace {

CF_CONNECTION_KEY InvalidConnectionKey() {
    CF_CONNECTION_KEY key = {};
    return key;
}

bool IsValidConnectionKey(const CF_CONNECTION_KEY& key) {
    CF_CONNECTION_KEY invalid = InvalidConnectionKey();
    return std::memcmp(&key, &invalid, sizeof(CF_CONNECTION_KEY)) != 0;
}

std::wstring RelativePathFromFullPath(const std::wstring& sync_root, const std::wstring& full_path) {
    // Windows cfapi normalized paths omit the drive letter (e.g. \Users\... instead of C:\Users\...)
    // Try matching with sync_root as-is first, then without the drive prefix
    std::wstring roots[] = { sync_root };
    if (sync_root.size() >= 2 && sync_root[1] == L':') {
        roots[0] = sync_root;
        // Also try without "C:" prefix
        std::wstring no_drive = sync_root.substr(2);
        for (const auto& root : {sync_root, no_drive}) {
            if (full_path.rfind(root, 0) == 0) {
                std::wstring relative = full_path.substr(root.size());
                if (!relative.empty() && (relative.front() == L'\\' || relative.front() == L'/')) {
                    relative.erase(relative.begin());
                }
                return relative;
            }
        }
        return full_path;
    }

    if (full_path.rfind(sync_root, 0) == 0) {
        std::wstring relative = full_path.substr(sync_root.size());
        if (!relative.empty() && (relative.front() == L'\\' || relative.front() == L'/')) {
            relative.erase(relative.begin());
        }
        return relative;
    }

    return full_path;
}

std::wstring ToRemotePath(const std::wstring& local_relative_path) {
    std::wstring remote = local_relative_path;
    std::replace(remote.begin(), remote.end(), L'\\', L'/');
    if (remote.empty() || remote.front() != L'/') {
        remote.insert(remote.begin(), L'/');
    }
    return remote;
}

}  // namespace

CloudProvider::CloudProvider(
    FilestashClient& client,
    StateStore& store,
    std::wstring sync_root)
    : client_(client),
      store_(store),
      sync_root_(std::move(sync_root)),
      connection_key_(InvalidConnectionKey()) {}

CloudProvider::~CloudProvider() {
    Disconnect();
}

void CloudProvider::RegisterSyncRoot() {
    CF_SYNC_REGISTRATION registration = {};
    registration.StructSize = sizeof(CF_SYNC_REGISTRATION);
    registration.ProviderName = L"Filestash";
    registration.ProviderVersion = L"1.0";

    CF_SYNC_POLICIES policies = {};
    policies.StructSize = sizeof(CF_SYNC_POLICIES);
    policies.Hydration.Primary = CF_HYDRATION_POLICY_FULL;
    policies.Population.Primary = CF_POPULATION_POLICY_FULL;

    HRESULT hr = CfRegisterSyncRoot(
        sync_root_.c_str(),
        &registration,
        &policies,
        CF_REGISTER_FLAG_NONE);

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to register sync root");
    }
    std::wcout << L"Sync root registered: " << sync_root_ << std::endl;
}

void CloudProvider::UnregisterSyncRoot() {
    CfUnregisterSyncRoot(sync_root_.c_str());
}

void CloudProvider::Connect() {
    CF_CALLBACK_REGISTRATION callbacks[] = {
        {CF_CALLBACK_TYPE_FETCH_DATA, OnFetchData},
        {CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS, OnFetchPlaceholders},
        {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, OnCancelFetchData},
        {CF_CALLBACK_TYPE_NONE, nullptr},
    };

    const HRESULT hr = CfConnectSyncRoot(
        sync_root_.c_str(),
        callbacks,
        this,
        CF_CONNECT_FLAG_NONE,
        &connection_key_);

    if (FAILED(hr)) {
        throw std::runtime_error("Failed to connect sync root");
    }
}

void CloudProvider::Disconnect() {
    if (!IsValidConnectionKey(connection_key_)) {
        return;
    }

    CfDisconnectSyncRoot(connection_key_);
    connection_key_ = InvalidConnectionKey();
}

void CALLBACK CloudProvider::OnFetchData(
    const CF_CALLBACK_INFO* callback_info,
    const CF_CALLBACK_PARAMETERS* callback_parameters) {
    auto* provider = static_cast<CloudProvider*>(callback_info->CallbackContext);
    const std::wstring full_path = callback_info->NormalizedPath;
    const std::wstring relative_path = RelativePathFromFullPath(provider->sync_root_, full_path);
    const std::wstring remote_path = ToRemotePath(relative_path);

    provider->PersistEvent(full_path, Event{EventType::HydrationRequested, L"", L""});

    std::wcout << L"FETCH_DATA local=" << full_path
               << L" remote=" << remote_path
               << L" offset=" << callback_parameters->FetchData.RequiredFileOffset.QuadPart
               << L" length=" << callback_parameters->FetchData.RequiredLength.QuadPart
               << std::endl;

    const auto content = provider->client_.ReadFile(remote_path);

    CF_OPERATION_INFO op_info = {};
    op_info.StructSize = sizeof(op_info);
    op_info.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
    op_info.ConnectionKey = callback_info->ConnectionKey;
    op_info.TransferKey = callback_info->TransferKey;

    CF_OPERATION_PARAMETERS op_params = {};
    op_params.ParamSize = sizeof(op_params);
    op_params.TransferData.CompletionStatus = STATUS_SUCCESS;
    op_params.TransferData.Buffer = content.empty() ? nullptr : content.data();
    op_params.TransferData.Offset.QuadPart = 0;
    op_params.TransferData.Length.QuadPart = static_cast<LONGLONG>(content.size());

    (void)CfExecute(&op_info, &op_params);

    provider->PersistEvent(full_path, Event{EventType::HydrationSucceeded, L"", L""});
}

void CALLBACK CloudProvider::OnFetchPlaceholders(
    const CF_CALLBACK_INFO* callback_info,
    const CF_CALLBACK_PARAMETERS* callback_parameters) {
    (void)callback_parameters;
    auto* provider = static_cast<CloudProvider*>(callback_info->CallbackContext);
    std::wcout << L"FETCH_PLACEHOLDERS path=" << callback_info->NormalizedPath << std::endl;

    std::wstring relative_path = RelativePathFromFullPath(
        provider->sync_root_, callback_info->NormalizedPath);
    provider->PopulateNamespace(relative_path);

    CF_OPERATION_INFO op_info = {};
    op_info.StructSize = sizeof(op_info);
    op_info.Type = CF_OPERATION_TYPE_TRANSFER_PLACEHOLDERS;
    op_info.ConnectionKey = callback_info->ConnectionKey;
    op_info.TransferKey = callback_info->TransferKey;

    CF_OPERATION_PARAMETERS op_params = {};
    op_params.ParamSize = sizeof(op_params);
    op_params.TransferPlaceholders.CompletionStatus = STATUS_SUCCESS;
    op_params.TransferPlaceholders.Flags = CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAG_DISABLE_ON_DEMAND_POPULATION;
    op_params.TransferPlaceholders.PlaceholderTotalCount.QuadPart = 0;

    (void)CfExecute(&op_info, &op_params);
}

void CALLBACK CloudProvider::OnCancelFetchData(
    const CF_CALLBACK_INFO* callback_info,
    const CF_CALLBACK_PARAMETERS* callback_parameters) {
    (void)callback_parameters;
    auto* provider = static_cast<CloudProvider*>(callback_info->CallbackContext);
    std::wcout << L"CANCEL_FETCH_DATA path=" << callback_info->NormalizedPath << std::endl;
    provider->PersistEvent(callback_info->NormalizedPath, Event{EventType::HydrationFailed, L"", L"Fetch cancelled"});
}

void CloudProvider::PopulateNamespace(const std::wstring& relative_path) {
    std::wstring remote_path = ToRemotePath(relative_path);
    std::wcout << L"[POPULATE] relative=" << relative_path
               << L" remote=" << remote_path << std::endl;
    auto entries = client_.ListDirectory(remote_path);
    std::wcout << L"[POPULATE] got " << entries.size() << L" entries" << std::endl;

    std::wstring local_dir = sync_root_;
    if (!relative_path.empty()) {
        local_dir += L"\\" + relative_path;
    }

    std::vector<std::wstring> names;
    std::vector<CF_PLACEHOLDER_CREATE_INFO> placeholders;
    names.reserve(entries.size());
    placeholders.reserve(entries.size());

    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    LARGE_INTEGER timestamp;
    timestamp.LowPart = now.dwLowDateTime;
    timestamp.HighPart = now.dwHighDateTime;

    for (const auto& entry : entries) {
        names.push_back(entry.name);

        CF_PLACEHOLDER_CREATE_INFO ph = {};
        ph.RelativeFileName = names.back().c_str();
        ph.FsMetadata.BasicInfo.CreationTime = timestamp;
        ph.FsMetadata.BasicInfo.LastWriteTime = timestamp;
        ph.FsMetadata.BasicInfo.LastAccessTime = timestamp;
        ph.FsMetadata.BasicInfo.ChangeTime = timestamp;
        ph.Flags = CF_PLACEHOLDER_CREATE_FLAG_NONE;

        if (entry.is_directory) {
            ph.FsMetadata.BasicInfo.FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            ph.FsMetadata.FileSize.QuadPart = 0;
        } else {
            ph.FsMetadata.BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
            ph.FsMetadata.FileSize.QuadPart = static_cast<LONGLONG>(entry.size);
        }

        placeholders.push_back(ph);
    }

    if (placeholders.empty()) {
        return;
    }

    DWORD processed = 0;
    HRESULT hr = CfCreatePlaceholders(
        local_dir.c_str(),
        placeholders.data(),
        static_cast<DWORD>(placeholders.size()),
        CF_CREATE_FLAG_NONE,
        &processed);

    if (FAILED(hr)) {
        std::wcerr << L"Failed to create placeholders in " << local_dir
                   << L" hr=0x" << std::hex << hr << std::dec << std::endl;
    } else {
        std::wcout << L"Created " << processed << L" placeholders in " << local_dir << std::endl;
    }
}

FileState CloudProvider::LoadOrCreateState(const std::wstring& local_path, const std::wstring& remote_path) {
    if (const auto existing = store_.Get(local_path)) {
        return *existing;
    }

    FileState created;
    created.local_path = local_path;
    created.remote_path = remote_path;
    return created;
}

void CloudProvider::PersistEvent(const std::wstring& local_path, const Event& event) {
    const std::wstring relative_path = RelativePathFromFullPath(sync_root_, local_path);
    FileState current = LoadOrCreateState(local_path, ToRemotePath(relative_path));
    FileState next = state_machine_.Apply(current, event);
    std::wcout << L"[EVENT] type=" << static_cast<int>(event.type)
               << L" path=" << relative_path << std::endl;
    store_.Put(next);
}
