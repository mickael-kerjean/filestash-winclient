#include "CloudProvider.h"

#include <windows.h>
#include <iostream>
#include <string>

struct Args {
    std::wstring url;
    std::wstring token;
};

HANDLE g_stop_event = NULL;

bool parseArgs(int argc, wchar_t* argv[], Args* args);
void printUsage(const wchar_t* program_name);
std::wstring getSyncRoot();
BOOL WINAPI ctrlHandler(DWORD ctrl_type);

int wmain(int argc, wchar_t* argv[]) {
    Args args;
    if (!parseArgs(argc, argv, &args)) {
        printUsage(argv[0]);
        return 1;
    }

    std::wstring sync_root = getSyncRoot();
    if (sync_root.empty()) {
        return 1;
    }

    FilestashClient client(args.url, args.token);
    MemoryStateStore store;
    CloudProvider provider(client, store, sync_root);

    provider.UnregisterSyncRoot();
    provider.RegisterSyncRoot();
    provider.Connect();
    provider.PopulateNamespace(L"");

    g_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler(ctrlHandler, TRUE);

    std::wcout << L"backend=" << client.base_url() << std::endl;
    std::wcout << L"sync-root=" << sync_root << std::endl;
    std::wcout << L"Press Ctrl+C to stop." << std::endl;

    WaitForSingleObject(g_stop_event, INFINITE);
    CloseHandle(g_stop_event);

    return 0;
}

bool parseArgs(int argc, wchar_t* argv[], Args* args) {
    for (int index = 1; index < argc; ++index) {
        const std::wstring arg = argv[index];
        if (arg == L"--url" && index + 1 < argc) {
            args->url = argv[++index];
        } else if (arg == L"--token" && index + 1 < argc) {
            args->token = argv[++index];
        }
    }
    return !args->url.empty() && !args->token.empty();
}

void printUsage(const wchar_t* program_name) {
    std::wcout << L"Usage: " << program_name
               << L" --url URL --token TOKEN" << std::endl;
}

std::wstring getSyncRoot() {
    wchar_t user_profile[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"USERPROFILE", user_profile, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        std::wcerr << L"Error: could not get USERPROFILE" << std::endl;
        return L"";
    }

    std::wstring path = std::wstring(user_profile) + L"\\Filestash";
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryW(path.c_str(), nullptr)) {
            std::wcerr << L"Error: could not create " << path << std::endl;
            return L"";
        }
    }

    return path;
}

BOOL WINAPI ctrlHandler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
        std::wcout << L"\nShutting down..." << std::endl;
        SetEvent(g_stop_event);
        return TRUE;
    }
    return FALSE;
}
