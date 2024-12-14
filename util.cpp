#pragma once
#include "util.h"
#include <shlobj.h>

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
void SearchLeftoverFiles(const std::vector<std::wstring>& paths) {
    for (const auto& path : paths) {
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile((path + L"\\*").c_str(), &findFileData);

        if (hFind == INVALID_HANDLE_VALUE) {
            std::wcout << L"Directory not found: " << path << std::endl;
            continue;
        }

        do {
            const std::wstring fileName = findFileData.cFileName;
            if (fileName != L"." && fileName != L"..") {
                std::wcout << L"Found leftover file/folder: " << path + L"\\" + fileName << std::endl;
            }
        } while (FindNextFile(hFind, &findFileData));

        FindClose(hFind);
    }
}

// Detect leftover registry keys
void SearchRegistryKeys(const std::vector<std::wstring>& registryPaths) {
    for (const auto& subKey : registryPaths) {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            WCHAR name[256];
            DWORD nameSize = sizeof(name) / sizeof(name[0]);
            DWORD index = 0;

            while (RegEnumKeyEx(hKey, index++, name, &nameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                std::wcout << L"Found leftover registry key: " << subKey + L"\\" + name << std::endl;
                nameSize = sizeof(name) / sizeof(name[0]);
            }

            RegCloseKey(hKey);
        }
        else {
            std::wcout << L"Registry key not found: " << subKey << std::endl;
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
                    std::wcout << L"Found leftover service: " << serviceName << std::endl;
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
void CleanUpLeftovers() {
    // Define known paths and registry locations
    std::vector<std::wstring> filePaths = {
        L"C:\\Program Files\\ProgramName",
        L"C:\\Users\\%USERNAME%\\AppData\\Local\\ProgramName",
        L"C:\\Users\\%USERNAME%\\AppData\\Roaming\\ProgramName"
    };

    std::vector<std::wstring> registryPaths = {
        L"SOFTWARE\\ProgramName",
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ProgramName"
    };

    // Detect leftover files and registry entries
    SearchLeftoverFiles(filePaths);
    SearchRegistryKeys(registryPaths);

    // Detect leftover services or processes
    SearchServicesAndProcesses(L"ProgramName");
}




