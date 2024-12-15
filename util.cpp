#pragma once
#include "util.h"
#include <shlobj.h>
#include <format>

ErrorDialog::ErrorDialog(wxWindow* parent, const wxString& message, const wxString& title)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(300, 200)),
	m_message(message), m_title(title) {
	// You can add other initialization code here
}

void ErrorDialog::ShowErrorMessage() const {
	wxMessageBox(m_message, m_title, wxICON_ERROR | wxOK);
}


void GetInstalledPrograms(std::map<wxString, wxString>& programs) {
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		wxLogError("Failed to open registry key for installed programs.");
		return;
	}

	char subKeyName[256];
	DWORD subKeyNameSize;
	DWORD index = 0;

	while (true) {
		subKeyNameSize = sizeof(subKeyName);
		if (RegEnumKeyExA(hKey, index, subKeyName, &subKeyNameSize, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
			break;
		}

		HKEY subKey;
		if (RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
			char displayName[256] = { 0 }, uninstallString[512] = { 0 };
			DWORD size;

			// Retrieve the display name
			size = sizeof(displayName);
			if (RegQueryValueExA(subKey, "DisplayName", nullptr, nullptr, (LPBYTE)displayName, &size) == ERROR_SUCCESS) {

				// Retrieve the uninstall string
				size = sizeof(uninstallString);
				if (RegQueryValueExA(subKey, "UninstallString", nullptr, nullptr, (LPBYTE)uninstallString, &size) == ERROR_SUCCESS) {

					wxString name = wxString::FromUTF8(displayName);
					wxString uninstall = wxString::FromUTF8(uninstallString);

					if (!name.IsEmpty() && !uninstall.IsEmpty()) {
						programs[name] = uninstall;
					}
				}
			}
			RegCloseKey(subKey);
		}
		index++;
	}

	RegCloseKey(hKey);
}

void runAsAdmin() {
	if (IsUserAnAdmin()) {
		wxLogInfo("Already running as administrator.");
		return;
	}

	wchar_t currentPath[MAX_PATH];

	if (GetModuleFileName(NULL, currentPath, MAX_PATH) == 0) {
		wxLogError("Failed to get current executable path!");
		return;
	}
	HINSTANCE result = ShellExecute(
		NULL,
		L"runas",
		currentPath,
		NULL,
		NULL,
		SW_SHOWNORMAL
	);

	if ((std::intptr_t)result <= 32) {
		wxLogError("Failed to relaunch with admin rights!");
	}
	else {
		wxLogInfo("Program relaunched with admin rights!");
		exit(0);
	}
}

// Detect leftover files
void SearchLeftoverFiles(const std::vector<std::wstring>& paths, const std::string program) {

    for (const auto& path : paths) {
        WIN32_FIND_DATA findFileData;
        std::string path_string(path.begin(), path.end());
        std::string fullPath = path_string + program;
        std::wstring wFullPath(fullPath.begin(), fullPath.end());
        wxLogInfo("Path with folder is: %s", wFullPath.c_str());

        HANDLE hFind = FindFirstFile(wFullPath.c_str(), &findFileData);

        if (hFind == INVALID_HANDLE_VALUE) {
            wxLogError("Directory not found %s", wFullPath.c_str());
            continue;
        }

        do {
            const std::wstring fileName = findFileData.cFileName;
            if (fileName != L"." && fileName != L"..") {
                wxLogInfo("Found leftover file/folder: %s, %s", path, fileName);
            }
        } while (FindNextFile(hFind, &findFileData));

        FindClose(hFind);
    }
}

// Detect leftover registry keys
void SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, char* programName) {
    int numberOfKeys = 0;
    for (const auto& subKey : registryPaths) {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            WCHAR name[256];
            DWORD nameSize = sizeof(name) / sizeof(name[0]);
            DWORD index = 0;

            while (RegEnumKeyExA(hKey, index++, programName, &nameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                numberOfKeys++;
                wxLogInfo("Found leftover registry key: %s, %s", subKey, programName);
            }
            wxLogInfo("Number of keys found is: %i", numberOfKeys);

            RegCloseKey(hKey);
        }
        else {
            wxLogError("Registry key not found: %s", subKey);
        }
    }
}

// Detect services and processes
void SearchServicesAndProcesses(const std::wstring& programName) {
    // Example: Enumerate services
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager) {
        DWORD bytesNeeded;
        DWORD serviceCount;
        DWORD resumeHandle = 0;

        EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE, NULL, 0, &bytesNeeded,
            &serviceCount, &resumeHandle, NULL);

        BYTE* buffer = new BYTE[bytesNeeded];
        if (EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE, buffer, bytesNeeded,
            &bytesNeeded, &serviceCount, &resumeHandle, NULL)) {
            LPENUM_SERVICE_STATUS_PROCESS services = (LPENUM_SERVICE_STATUS_PROCESS)buffer;

            for (DWORD i = 0; i < serviceCount; i++) {
                std::wstring serviceName = services[i].lpServiceName;
                if (serviceName.find(programName) != std::wstring::npos) {
                    wxLogInfo("Found leftover service: %s", serviceName);
                }
            }
        }
        delete[] buffer;
        CloseServiceHandle(hSCManager);
    }

    // Example: Enumerate processes (using EnumProcesses or ToolHelp APIs)
    // Add similar logic to check for related processes.
}

// Cleanup leftovers 
void CleanUpLeftovers(std::string programName, char* regPath) {
    // Define known paths and registry locations
    std::vector<std::wstring> filePaths = {
        L"C:\\Program Files\\",
        L"C:\\Users\\%USERNAME%\\AppData\\Local\\",
        L"C:\\Users\\%USERNAME%\\AppData\\Roaming\\"
    };

    std::vector<std::wstring> registryPaths = {
        L"SOFTWARE\\",
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
    };

    // Detect leftover files and registry entries
    SearchLeftoverFiles(filePaths, programName);
    SearchRegistryKeys(registryPaths, regPath);

    // Detect leftover services or processes
    SearchServicesAndProcesses(L"ProgramName");
}




