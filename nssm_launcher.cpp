#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <ShellScalingApi.h> 
#include <string>
#include <vector>
#include "resource.h"

#include "nssm_settings.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shcore.lib")

struct ServiceInfo {
    std::wstring name;          // Service name (registry key)
    std::wstring displayName;   // Display name
    std::wstring executable;    // Actual executable path
    std::wstring nssmPath;      // Path to NSSM executable managing this service
};

// Launch NSSM with the given command and service name
bool LaunchNSSM(const wchar_t* nssmPath, const wchar_t* command, const wchar_t* serviceName) {
    std::wstring cmdLine = L"\"";
    cmdLine += nssmPath;
    cmdLine += L"\" ";
    cmdLine += command;
    cmdLine += L" \"";
    cmdLine += serviceName;
    cmdLine += L"\"";

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

// Get list of NSSM services from registry
std::vector<ServiceInfo> GetNSSMServices() {
    std::vector<ServiceInfo> services;
    HKEY servicesKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services",
        0, KEY_READ, &servicesKey) != ERROR_SUCCESS) {
        return services;
    }

    wchar_t serviceName[256];
    DWORD index = 0;
    DWORD nameLen = sizeof(serviceName) / sizeof(wchar_t);

    while (RegEnumKeyExW(servicesKey, index++, serviceName, &nameLen,
        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        // Open this service's key
        HKEY serviceKey;
        if (RegOpenKeyExW(servicesKey, serviceName, 0, KEY_READ, &serviceKey) == ERROR_SUCCESS) {
            wchar_t imagePath[MAX_PATH];
            DWORD pathSize = sizeof(imagePath);

            // Check if this is an NSSM service by looking at ImagePath
            if (RegQueryValueExW(serviceKey, L"ImagePath", NULL, NULL,
                (LPBYTE)imagePath, &pathSize) == ERROR_SUCCESS) {

                std::wstring imagePathStr = imagePath;
                if (imagePathStr.find(L"nssm.exe") != std::wstring::npos) {
                    ServiceInfo info;
                    info.name = serviceName;
                    info.nssmPath = imagePath;

                    // Get display name
                    wchar_t displayName[256];
                    DWORD displayNameSize = sizeof(displayName);
                    if (RegQueryValueExW(serviceKey, L"DisplayName", NULL, NULL,
                        (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {
                        info.displayName = displayName;
                    }

                    // Get actual executable from Parameters/Application
                    HKEY paramsKey;
                    if (RegOpenKeyExW(serviceKey, L"Parameters", 0, KEY_READ, &paramsKey) == ERROR_SUCCESS) {
                        wchar_t application[MAX_PATH];
                        DWORD appSize = sizeof(application);
                        if (RegQueryValueExW(paramsKey, L"Application", NULL, NULL,
                            (LPBYTE)application, &appSize) == ERROR_SUCCESS) {
                            info.executable = application;
                        }
                        RegCloseKey(paramsKey);
                    }

                    services.push_back(info);
                }
            }
            RegCloseKey(serviceKey);
        }
        nameLen = sizeof(serviceName) / sizeof(wchar_t);
    }

    RegCloseKey(servicesKey);
    return services;
}

// Initialize ListView columns
void InitListView(HWND hList) {
    // Set extended styles for full row selection and grid lines
    DWORD exStyle = ListView_GetExtendedListViewStyle(hList);
    exStyle |= LVS_EX_FULLROWSELECT;  // Makes the whole row selectable
    exStyle |= LVS_EX_GRIDLINES;      // Optional: adds grid lines
    ListView_SetExtendedListViewStyle(hList, exStyle);

    LVCOLUMN lvc = { 0 };
    wchar_t col1[] = L"Service Name";
    wchar_t col2[] = L"Display Name";
    wchar_t col3[] = L"Executable";

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0;
    lvc.cx = 100;
    lvc.pszText = col1;
    ListView_InsertColumn(hList, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx = 150;
    lvc.pszText = col2;
    ListView_InsertColumn(hList, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.cx = 200; // LVSCW_AUTOSIZE_USEHEADER;   // This makes it extend
    lvc.pszText = col3;
    ListView_InsertColumn(hList, 2, &lvc);

    // Now set the last column to fill remaining space
    RECT rc;
    GetClientRect(hList, &rc);  // Get the list view's client area

    // Calculate the width of the first two columns
    int totalWidth = ListView_GetColumnWidth(hList, 0) +
        ListView_GetColumnWidth(hList, 1);

    // Set the last column width to fill the remaining space
    // We subtract 4 to account for the borders and prevent horizontal scrollbar
    ListView_SetColumnWidth(hList, 2, rc.right - totalWidth - 4);
}

// Fill ListView with services
void PopulateList(HWND hList, const std::vector<ServiceInfo>& services) {
    ListView_DeleteAllItems(hList);

    for (size_t i = 0; i < services.size(); i++) {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;

        // Service name
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)services[i].name.c_str();
        ListView_InsertItem(hList, &lvi);

        // Display name
        lvi.iSubItem = 1;
        lvi.pszText = (LPWSTR)services[i].displayName.c_str();
        ListView_SetItem(hList, &lvi);

        // Executable
        lvi.iSubItem = 2;
        lvi.pszText = (LPWSTR)services[i].executable.c_str();
        ListView_SetItem(hList, &lvi);
    }
}

// Dialog procedure
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static std::vector<ServiceInfo> services;
    static HWND hList;

    switch (msg) {
    case WM_INITDIALOG: {
        // Initialize common controls
        INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX) };
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        // Get handles
        hList = GetDlgItem(hwnd, IDC_SERVICE_LIST);

        // Setup list view
        InitListView(hList);

        // Get and show services
        services = GetNSSMServices();
        PopulateList(hList, services);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_NEW: {
            wchar_t serviceName[256];
            GetDlgItemTextW(hwnd, IDC_NEW_SERVICE, serviceName, 256);
            if (serviceName[0] == 0) {
                MessageBoxW(hwnd, L"Please enter a service name.", L"Error", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            // Check if service exists
            for (const auto& svc : services) {
                if (_wcsicmp(svc.name.c_str(), serviceName) == 0) {
                    MessageBoxW(hwnd, L"A service with this name already exists.",
                        L"Error", MB_OK | MB_ICONERROR);
                    return TRUE;
                }
            }

            // Launch NSSM install
            if (!LaunchNSSM(L"c:\\src\\nssm\\out\\Debug\\win32\\nssm.exe",
                L"install", serviceName)) {
                MessageBoxW(hwnd, L"Failed to launch NSSM.", L"Error", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            // Refresh list after a brief delay to allow NSSM to finish
            Sleep(1000);
            services = GetNSSMServices();
            PopulateList(hList, services);
            return TRUE;
        }

        case IDC_EDIT: {
            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (sel == -1) {
                MessageBoxW(hwnd, L"Please select a service to edit.",
                    L"Error", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            if (!LaunchNSSM(services[sel].nssmPath.c_str(), L"edit",
                services[sel].name.c_str())) {
                MessageBoxW(hwnd, L"Failed to launch NSSM.", L"Error", MB_OK | MB_ICONERROR);
            }
            return TRUE;
        }

        case IDC_SETTINGS: {
            settings::show_dialog(hwnd);
            break;
        }

        case IDCANCEL:
            EndDialog(hwnd, 0);
            return TRUE;
        }
    
        break;

    case WM_NOTIFY: {
        NMHDR* pnmh = (NMHDR*)lParam;
        if (pnmh->idFrom == IDC_SERVICE_LIST && pnmh->code == NM_DBLCLK) {
            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_EDIT, 0), 0);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DialogProc);
    return 0;
}