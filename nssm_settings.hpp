#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filesystem>
#include <optional>

namespace settings {
    // Shows the settings dialog modally. Returns true if OK was pressed
    bool show_dialog(HWND parent);

    // Gets the currently configured NSSM path from registry
    // Returns std::nullopt if not configured
    std::optional<std::filesystem::path> get_nssm_path();

    // Sets the NSSM path in registry
    // Throws std::runtime_error if registry operation fails
    void set_nssm_path(const std::filesystem::path& path);
}