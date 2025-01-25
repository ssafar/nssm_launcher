#include "nssm_settings.hpp"

#include "resource.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <stdexcept>
#include <commdlg.h>

namespace {
    constexpr const wchar_t* REG_KEY_PATH = L"SOFTWARE\\Simon Safar\\NSSM Launcher";
    constexpr const wchar_t* REG_VALUE_NAME = L"NssmPath";

    // Dialog procedure for the settings dialog
    INT_PTR CALLBACK settings_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
        case WM_INITDIALOG: {
            // Center dialog relative to parent
            HWND parent = GetParent(hwnd);
            if (parent) {
                RECT rc_parent, rc_dlg, rc_centered;
                GetWindowRect(parent, &rc_parent);
                GetWindowRect(hwnd, &rc_dlg);

                int dlg_width = rc_dlg.right - rc_dlg.left;
                int dlg_height = rc_dlg.bottom - rc_dlg.top;

                rc_centered.left = rc_parent.left + (rc_parent.right - rc_parent.left - dlg_width) / 2;
                rc_centered.top = rc_parent.top + (rc_parent.bottom - rc_parent.top - dlg_height) / 2;

                SetWindowPos(hwnd, nullptr, rc_centered.left, rc_centered.top, 0, 0,
                    SWP_NOSIZE | SWP_NOZORDER);
            }

            // Load current path if any
            if (auto path = settings::get_nssm_path()) {
                SetDlgItemText(hwnd, IDC_NSSM_PATH, path->c_str());
            }
            return TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wparam)) {
            case IDC_BROWSE_NSSM: {
                wchar_t filename[MAX_PATH] = { 0 };

                OPENFILENAME ofn = { 0 };
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"NSSM Executable (nssm.exe)\0nssm.exe\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.lpstrTitle = L"Select NSSM Executable";
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                if (GetOpenFileName(&ofn)) {
                    SetDlgItemText(hwnd, IDC_NSSM_PATH, filename);
                }
                return TRUE;
            }

            case IDOK: {
                wchar_t path[MAX_PATH];
                GetDlgItemText(hwnd, IDC_NSSM_PATH, path, MAX_PATH);

                try {
                    settings::set_nssm_path(path);
                    EndDialog(hwnd, IDOK);
                }
                catch (const std::runtime_error& e) {
                    MessageBoxA(hwnd, e.what(), "Error", MB_ICONERROR | MB_OK);
                }
                return TRUE;
            }

            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            break;
        }
        }
        return FALSE;
    }
}

namespace settings {
    bool show_dialog(HWND parent) {
        return DialogBox(GetModuleHandle(nullptr),
            MAKEINTRESOURCE(IDD_SETTINGS),
            parent,
            settings_dlg_proc) == IDOK;
    }

    std::optional<std::filesystem::path> get_nssm_path() {
        HKEY key;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &key) != ERROR_SUCCESS) {
            return std::nullopt;
        }

        std::unique_ptr<HKEY__, decltype(&RegCloseKey)> key_guard(key, RegCloseKey);

        wchar_t path[MAX_PATH];
        DWORD size = sizeof(path);
        DWORD type;

        if (RegQueryValueEx(key, REG_VALUE_NAME, nullptr, &type,
            reinterpret_cast<BYTE*>(path), &size) != ERROR_SUCCESS
            || type != REG_SZ) {
            return std::nullopt;
        }

        // Check for emptiness
        if (path[0] == L'\0') {
            return std::nullopt;
        }
       
        return std::filesystem::path(path);
    }

    void set_nssm_path(const std::filesystem::path& path) {
        HKEY key;
        DWORD disposition;

        // Create/open the key
        if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_KEY_PATH, 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
            &key, &disposition) != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to create/open registry key");
        }

        std::unique_ptr<HKEY__, decltype(&RegCloseKey)> key_guard(key, RegCloseKey);

        // Store the path
        auto path_str = path.wstring();
        if (RegSetValueEx(key, REG_VALUE_NAME, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(path_str.c_str()),
            static_cast<DWORD>((path_str.length() + 1) * sizeof(wchar_t)))
            != ERROR_SUCCESS) {
            throw std::runtime_error("Failed to write to registry");
        }
    }
}