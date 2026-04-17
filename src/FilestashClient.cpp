#include "FilestashClient.h"

FilestashClient::FilestashClient(std::wstring base_url, std::wstring token)
    : base_url_(std::move(base_url)), token_(std::move(token)) {}

const std::wstring& FilestashClient::base_url() const {
    return base_url_;
}

std::vector<RemoteEntry> FilestashClient::ListDirectory(const std::wstring& remote_path) const {
    (void)remote_path;
    return {};
}

std::vector<std::uint8_t> FilestashClient::ReadFile(const std::wstring& remote_path) const {
    (void)remote_path;
    return {};
}
