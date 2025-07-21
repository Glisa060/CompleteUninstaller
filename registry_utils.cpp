#include "registry_utils.h"
// Returns the root HKEY from the input full registry path string.
// 'subKey' will contain the rest of the key path.
// Returns nullptr on failure.
HKEY GetRootKeyFromString(const std::wstring& fullPath, std::wstring& subKey)
{
    static const std::pair<std::wstring, HKEY> roots[] = {
        {L"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE},
        {L"HKEY_CURRENT_USER", HKEY_CURRENT_USER},
        {L"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT},
        {L"HKEY_USERS", HKEY_USERS},
        {L"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG}
    };

    for (const auto& [name, hive] : roots) {
        if (fullPath.compare(0, name.length(), name) == 0) {
            // Expect backslash after root name, or full path equals root name only
            if (fullPath.length() == name.length()) {
                subKey.clear();
                return hive;
            }
            else if (fullPath[name.length()] == L'\\') {
                subKey = fullPath.substr(name.length() + 1); // skip backslash
                return hive;
            }
        }
    }

    // Not a recognized root hive
    subKey.clear();
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

static void RecursiveSearchRegistry(
    HKEY hKey,
    const std::wstring& currentPath,
    const std::wstring& progNameW,
    std::mutex& foundKeysMutex,
    std::vector<std::wstring>& foundKeys)
{
    DWORD subKeyCount = 0, maxKeyNameLen = 0, valueCount = 0, maxValueNameLen = 0, maxValueLen = 0;
    if (RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &subKeyCount, &maxKeyNameLen, NULL,
        &valueCount, &maxValueNameLen, &maxValueLen, NULL, NULL) != ERROR_SUCCESS) {
        return;
    }

    std::vector<WCHAR> keyNameBuffer(maxKeyNameLen + 1);
    for (DWORD i = 0; i < subKeyCount; ++i) {
        DWORD keyNameLen = maxKeyNameLen + 1;
        LONG result = RegEnumKeyExW(hKey, i, keyNameBuffer.data(), &keyNameLen, NULL, NULL, NULL, NULL);
        if (result != ERROR_SUCCESS) continue;

        std::wstring nameStr(keyNameBuffer.data(), keyNameLen);
        std::wstring lowerName = nameStr;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

        if (lowerName.find(progNameW) != std::wstring::npos) {
            std::wstring fullFoundKey = currentPath + L"\\" + nameStr;
            {
                std::lock_guard<std::mutex> lock(foundKeysMutex);
                foundKeys.push_back(fullFoundKey);
            }
            wxLogInfo("Found leftover registry key (name match): %s", wxString(fullFoundKey));
        }

        HKEY subKeyHandle;
        if (RegOpenKeyExW(hKey, keyNameBuffer.data(), 0, KEY_READ, &subKeyHandle) == ERROR_SUCCESS) {
            RecursiveSearchRegistry(subKeyHandle, currentPath + L"\\" + nameStr, progNameW, foundKeysMutex, foundKeys);
            RegCloseKey(subKeyHandle);
        }
    }

    std::vector<WCHAR> valueNameBuffer(maxValueNameLen + 1);
    std::vector<BYTE> valueDataBuffer(maxValueLen);

    for (DWORD i = 0; i < valueCount; ++i) {
        DWORD valueNameLen = maxValueNameLen + 1;
        DWORD dataSize = maxValueLen;
        DWORD type;

        LONG result = RegEnumValueW(hKey, i, valueNameBuffer.data(), &valueNameLen, NULL, &type, valueDataBuffer.data(), &dataSize);
        if (result != ERROR_SUCCESS) continue;

        bool match = false;

        if (type == REG_SZ || type == REG_EXPAND_SZ) {
            // Construct string safely (stop at null terminator)
            std::wstring valueStr(reinterpret_cast<wchar_t*>(valueDataBuffer.data()));
            std::wstring lowerVal = valueStr;
            std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::towlower);
            if (lowerVal.find(progNameW) != std::wstring::npos) {
                match = true;
            }
        }
        else if (type == REG_MULTI_SZ) {
            const wchar_t* str = reinterpret_cast<wchar_t*>(valueDataBuffer.data());
            while (*str) {
                std::wstring entry(str);
                std::wstring lowerEntry = entry;
                std::transform(lowerEntry.begin(), lowerEntry.end(), lowerEntry.begin(), ::towlower);
                if (lowerEntry.find(progNameW) != std::wstring::npos) {
                    match = true;
                    break;
                }
                str += entry.length() + 1;
            }
        }

        if (match) {
            {
                std::lock_guard<std::mutex> lock(foundKeysMutex);
                foundKeys.push_back(currentPath);
            }
            wxLogInfo("Found leftover registry key (value match): %s", wxString(currentPath));
            break;  // Found a match, no need to check further values for this key
        }
    }
}


// Search registry keys in both HKLM and HKCU under given relative paths for a program name.
// Case insensitive search.
// Returns vector of full key paths including root prefix (e.g. HKLM\...).
// Mutex for thread-safe access to shared vector
static std::mutex foundKeysMutex;

std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const wxString& programName)
{
    std::vector<std::wstring> foundKeys;
    std::wstring progNameW(programName.begin(), programName.end());
    std::transform(progNameW.begin(), progNameW.end(), progNameW.begin(), ::towlower);

    std::vector<std::thread> workers;

    for (const auto& fullKeyPath : registryPaths) {
        if (fullKeyPath.empty()) {
            wxLogWarning("Registry path is empty!");
            continue;
        }

        wxLogMessage("Registry path: %s", wxString(fullKeyPath));

        // Capture foundKeysMutex by reference, also foundKeys by reference
        workers.emplace_back([fullKeyPath, &progNameW, &foundKeys]() {
            std::wstring subKey;
            HKEY root = GetRootKeyFromString(fullKeyPath, subKey);

            if (root == nullptr) {
                wxLogWarning("Invalid root in registry path: %s", wxString(fullKeyPath));
                return;
            }

            HKEY hKey;
            if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                wxLogWarning("Registry path inaccessible or not found: %s", wxString(fullKeyPath));
                return;
            }

            // use foundKeysMutex directly
            RecursiveSearchRegistry(hKey, fullKeyPath, progNameW, foundKeysMutex, foundKeys);

            RegCloseKey(hKey);
        });
    }

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    return foundKeys;
}


std::optional<wxString> FindRegistryPathForProgram(
    const std::map<wxString, wxString>& registryPaths,
    const wxString& normalizedName)
{
    wxString lowered = normalizedName.Lower();

    for (const auto& [key, value] : registryPaths) {
        wxString keyLower = key.Lower();

        if (keyLower.Contains(lowered) && !value.IsEmpty()) {
            return value;
        }
    }
    return std::nullopt;
}

void ReadProgramsFromRegistry(HKEY root, const std::string& path,
    std::map<wxString, wxString>& programs,
    std::map<wxString, wxString>& manufacturers)
{
    HKEY hKey;
    if (RegOpenKeyExA(root, path.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return;
    }

    char subKeyName[256];
    DWORD subKeyNameSize;
    DWORD index = 0;

    while (true) {
        subKeyNameSize = sizeof(subKeyName);
        if (RegEnumKeyExA(hKey, index++, subKeyName, &subKeyNameSize, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
            break;

        HKEY subKey;
        if (RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
            char displayName[256] = { 0 }, uninstallString[512] = { 0 }, publisher[256] = { 0 };
            DWORD size;

            size = sizeof(displayName);
            if (RegQueryValueExA(subKey, "DisplayName", nullptr, nullptr, (LPBYTE)displayName, &size) == ERROR_SUCCESS) {
                size = sizeof(uninstallString);
                RegQueryValueExA(subKey, "UninstallString", nullptr, nullptr, (LPBYTE)uninstallString, &size);

                size = sizeof(publisher);
                RegQueryValueExA(subKey, "Publisher", nullptr, nullptr, (LPBYTE)publisher, &size);

                wxString name = wxString::FromUTF8(displayName);
                wxString uninstall = wxString::FromUTF8(uninstallString);
                wxString mfg = wxString::FromUTF8(publisher);

                // add full regPath including (etc. WOW6432Node...)
                wxString fullRegPath = wxString::Format("HKEY_%s\\%s\\%s",
                    (root == HKEY_LOCAL_MACHINE ? "LOCAL_MACHINE" : "CURRENT_USER"),
                    wxString(path),
                    wxString::FromUTF8(subKeyName));

                if (!name.IsEmpty()) {
                    programs[name] = fullRegPath;
                    if (!mfg.IsEmpty()) {
                        manufacturers[name] = mfg;
                    }
                }
            }

            RegCloseKey(subKey);
        }
    }

    RegCloseKey(hKey);
}



std::string GetRegistryPathForProgram(const std::string& name) {
    auto it = registryPaths.find(name);
    return (it != registryPaths.end()) ? std::string(it->second.mb_str()) : std::string();
}

void GetInstalledPrograms(std::map<wxString, wxString>& programs, std::map<wxString, wxString>& manufacturers) {
    ReadProgramsFromRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", programs, manufacturers);
    ReadProgramsFromRegistry(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", programs, manufacturers);
    ReadProgramsFromRegistry(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", programs, manufacturers);
}