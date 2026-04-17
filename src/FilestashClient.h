#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct RemoteEntry {
    std::wstring name;
    bool is_directory = false;
    std::uint64_t size = 0;
    std::wstring modified_at;
};

class FilestashClient {
public:
    FilestashClient(std::wstring base_url, std::wstring token);

    const std::wstring& base_url() const;

    std::vector<RemoteEntry> ListDirectory(const std::wstring& remote_path) const;
    std::vector<std::uint8_t> ReadFile(const std::wstring& remote_path) const;

private:
    std::wstring base_url_;
    std::wstring token_;
};
