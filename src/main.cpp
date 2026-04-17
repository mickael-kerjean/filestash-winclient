#include "CloudProvider.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

struct Args {
    std::wstring url;
    std::wstring token;
    std::wstring sync_root;
};

bool ParseArgs(int argc, wchar_t* argv[], Args* args) {
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

void PrintUsage(const wchar_t* program_name) {
    std::wcout << L"Usage: " << program_name
               << L" --url URL --token TOKEN --sync-root PATH" << std::endl;
}

}  // namespace

int wmain(int argc, wchar_t* argv[]) {
    Args args;
    if (!ParseArgs(argc, argv, &args)) {
        PrintUsage(argv[0]);
        return 1;
    }

    auto client = std::make_shared<FilestashClient>(args.url, args.token);
    auto store = std::make_shared<MemoryStateStore>();
    CloudProvider provider(client, store, args.sync_root);

    std::wcout << L"filestash-cfapi scaffold" << std::endl;
    std::wcout << L"backend=" << client->base_url() << std::endl;
    std::wcout << L"sync-root=" << args.sync_root << std::endl;
    std::wcout << L"state-store=memory (SQLite planned)" << std::endl;

    return 0;
}
