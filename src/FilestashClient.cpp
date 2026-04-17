#include "FilestashClient.h"

#include <cstring>

FilestashClient::FilestashClient(std::wstring base_url, std::wstring token)
    : base_url_(std::move(base_url)), token_(std::move(token)) {}

const std::wstring& FilestashClient::base_url() const {
    return base_url_;
}

std::vector<RemoteEntry> FilestashClient::ListDirectory(const std::wstring& remote_path) const {
    if (remote_path == L"/") {
        return {
            {L"Documents", true, 0, L""},
            {L"Photos", true, 0, L""},
            {L"hello.txt", false, 13, L""},
            {L"notes.txt", false, 21, L""},
        };
    }
    if (remote_path == L"/Documents") {
        return {
            {L"report.txt", false, 45, L""},
            {L"todo.txt", false, 30, L""},
        };
    }
    if (remote_path == L"/Photos") {
        return {
            {L"vacation", true, 0, L""},
            {L"cat.txt", false, 19, L""},
        };
    }
    if (remote_path == L"/Photos/vacation") {
        return {
            {L"beach.txt", false, 24, L""},
        };
    }
    return {};
}

std::vector<std::uint8_t> FilestashClient::ReadFile(const std::wstring& remote_path) const {
    auto text = [](const char* s) {
        return std::vector<std::uint8_t>(s, s + std::strlen(s));
    };

    if (remote_path == L"/hello.txt") return text("Hello, World!");
    if (remote_path == L"/notes.txt") return text("Some notes go here.");
    if (remote_path == L"/Documents/report.txt") return text("This is a report about cloud sync providers.");
    if (remote_path == L"/Documents/todo.txt") return text("1. Implement sync\n2. Ship it");
    if (remote_path == L"/Photos/cat.txt") return text("A cute cat picture");
    if (remote_path == L"/Photos/vacation/beach.txt") return text("Waves crashing on shore");

    return {};
}
