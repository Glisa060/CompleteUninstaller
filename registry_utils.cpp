#include <wx/log.h>
#include <wx/busyinfo.h>
#include <algorithm>
#include "registry_utils.h"



// Helper function to get root HKEY and subkey path from a full registry path string.
// Supports prefixes like "HKLM\", "HKEY_LOCAL_MACHINE\", "HKCU\", "HKEY_CURRENT_USER\".
static HKEY GetRootKeyFromString(const std::wstring& fullKeyPath, std::wstring& subKeyOut) {
    if (fullKeyPath.compare(0, 5, L"HKLM\\") == 0) {
        subKeyOut = fullKeyPath.substr(5);
        return HKEY_LOCAL_MACHINE;
    }
    if (fullKeyPath.compare(0, 18, L"HKEY_LOCAL_MACHINE\\") == 0) {
        subKeyOut = fullKeyPath.substr(18);
        return HKEY_LOCAL_MACHINE;
    }
    if (fullKeyPath.compare(0, 5, L"HKCU\\") == 0) {
        subKeyOut = fullKeyPath.substr(5);
        return HKEY_CURRENT_USER;
    }
    if (fullKeyPath.compare(0, 17, L"HKEY_CURRENT_USER\\") == 0) {
        subKeyOut = fullKeyPath.substr(17);
        return HKEY_CURRENT_USER;
    }
    // If root not recognized, return nullptr and output full string as subKey.
    subKeyOut = fullKeyPath;
    return nullptr;
}

// Recursively deletes a registry key and all its subkeys.
// hKeyRoot is the root key like HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER.
// subKey is the relative path under the root.
LONG RegDeleteKeyRecursive(HKEY hKeyRoot, const std::wstring& subKey) {
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey);
    if (lRes != ERROR_SUCCESS) {
        return lRes;
    }

    WCHAR name[256];
    DWORD size = 256;

    // Enumerate all subkeys and delete them recursively.
    while (RegEnumKeyExW(hKey, 0, name, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        std::wstring sub(subKey + L"\\" + name);
        RegDeleteKeyRecursive(hKeyRoot, sub);
        size = 256;  // reset buffer size for next iteration
    }

    RegCloseKey(hKey);
    // Finally, delete the key itself.
    return RegDeleteKeyW(hKeyRoot, subKey.c_str());
}

// Overload for std::wstring full path: extract root and delete recursively.
LONG RegDeleteKeyRecursiveByPath(const std::wstring& fullKeyPath) {
    std::wstring subKey;
    HKEY root = GetRootKeyFromString(fullKeyPath, subKey);
    if (!root) {
        wxLogError("Invalid root key in registry path: %s", wxString(fullKeyPath));
        return ERROR_INVALID_PARAMETER;
    }
    return RegDeleteKeyRecursive(root, subKey);
}

// Overload for std::string path.
LONG RegDeleteKeyRecursiveByPath(const std::string& fullKeyPath) {
    return RegDeleteKeyRecursiveByPath(std::wstring(fullKeyPath.begin(), fullKeyPath.end()));
}

// Overload for wxString path.
LONG RegDeleteKeyRecursiveByPath(const wxString& fullKeyPath) {
    return RegDeleteKeyRecursiveByPath(fullKeyPath.ToStdWstring());
}

// Search registry keys in both HKLM and HKCU under given relative paths for a program name.
// Case insensitive search.
// Returns vector of full key paths including root prefix (e.g. HKLM\...).
std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const wxString& programName) {
    wxBusyInfo info("Searching registry keys...");
    std::vector<std::wstring> foundKeys;

    // Convert program name to lowercase for case-insensitive matching.
    std::wstring progNameW(programName.begin(), programName.end());
    std::transform(progNameW.begin(), progNameW.end(), progNameW.begin(), ::towlower);

    for (const auto& subKey : registryPaths) {
        // Search in both HKLM and HKCU for the given subkey.
        std::vector<HKEY> roots = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
        for (auto root : roots) {
            HKEY hKey;
            if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                wxLogWarning("Registry path inaccessible or not found: %s", wxString(subKey));
                continue;
            }

            DWORD index = 0;
            WCHAR keyName[256];

            // Enumerate subkeys under current registry path.
            while (true) {
                DWORD keyNameLen = 256;
                LONG result = RegEnumKeyExW(hKey, index++, keyName, &keyNameLen, NULL, NULL, NULL, NULL);
                if (result == ERROR_NO_MORE_ITEMS) break;
                if (result != ERROR_SUCCESS) continue;

                std::wstring nameStr(keyName);
                std::wstring lowerName = nameStr;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

                // Check if the subkey contains the program name substring.
                if (lowerName.find(progNameW) != std::wstring::npos) {
                    wchar_t rootStr[20];
                    if (root == HKEY_LOCAL_MACHINE) {
                        wcscpy(rootStr, L"HKLM");
                    }
                    else if (root == HKEY_CURRENT_USER) {
                        wcscpy(rootStr, L"HKCU");
                    }
                    else {
                        wcscpy(rootStr, L"UNKNOWN");
                    }

                    // Build full key path with root prefix.
                    std::wstring fullKey = std::wstring(rootStr) + L"\\" + subKey + L"\\" + nameStr;
                    foundKeys.push_back(fullKey);
                    wxLogInfo("Found leftover registry key: %s", wxString(fullKey));
                }
            }
            RegCloseKey(hKey);
        }
    }

    return foundKeys;
}
