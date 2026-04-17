#include "CloudProvider.h"

#include <windows.h>
#include <iostream>
#include <string>

struct Args {
    std::wstring url;
    std::wstring token;
    std::wstring sync_root;
};

HANDLE g_stop_event = NULL;

bool parseArgs(int argc, wchar_t* argv[], Args* args);
void printUsage(const wchar_t* program_name);
BOOL WINAPI ctrlHandler(DWORD ctrl_type);

int wmain(int argc, wchar_t* argv[]) {
    Args args;
    if (!ParseArgs(argc, argv, &args)) {
        PrintUsage(argv[0]);
        return 1;
    }

    FilestashClient client(args.url, args.token);
    MemoryStateStore store;
    CloudProvider provider(client, store, args.sync_root);

    g_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler(ctrlHandler, TRUE);

    std::wcout << L"filestash-cfapi scaffold" << std::endl;
    std::wcout << L"backend=" << client.base_url() << std::endl;
    std::wcout << L"sync-root=" << args.sync_root << std::endl;
    std::wcout << L"state-store=memory (SQLite planned)" << std::endl;
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
        } else if (arg == L"--sync-root" && index + 1 < argc) {
            args->sync_root = argv[++index];
        }
    }

    return !args->url.empty() && !args->token.empty() && !args->sync_root.empty();
}

void printUsage(const wchar_t* program_name) {
    std::wcout << L"Usage: " << program_name
               << L" --url URL --token TOKEN --sync-root PATH" << std::endl;
}

BOOL WINAPI ctrlHandler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
        std::wcout << L"\nShutting down..." << std::endl;
        SetEvent(g_stop_event);
        return TRUE;
    }
    return FALSE;
}
