#include "util.h"


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

std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId) {
	PWSTR path = nullptr;
	std::wstring result;

	if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, NULL, &path))) {
		result = path;
		CoTaskMemFree(path);
	}

	return result;
}

std::wstring GetProgramFilesDir() {
	WCHAR path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path))) {
		return std::wstring(path);
	}
	return L"C:\\Program Files";
}

std::wstring GetProgramFilesX86Dir() {
	WCHAR path[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, path))) {
		return std::wstring(path);
	}
	return L"C:\\Program Files (x86)";
}
void runAsAdmin(wxFrame* mainFrame = nullptr) {
	if (IsUserAnAdmin()) {
		wxLogInfo("Already running as administrator.");
		return;
	}

	wchar_t currentPath[MAX_PATH];
	if (GetModuleFileNameW(NULL, currentPath, MAX_PATH) == 0) {
		wxLogError("Failed to get current executable path!");
		return;
	}

	SHELLEXECUTEINFOW sei = { sizeof(sei) };
	sei.lpVerb = L"runas";
	sei.lpFile = currentPath;
	sei.nShow = SW_SHOWNORMAL;
	sei.fMask = SEE_MASK_NOASYNC;

	if (!ShellExecuteExW(&sei)) {
		DWORD err = GetLastError();
		wxLogError("Failed to relaunch with admin rights! Error code: %lu", err);
		return;
	}

	wxLogInfo("Program relaunched with admin rights. Exiting current instance...");

	// Umesto exit(0), koristi bezbedno zatvaranje:
	if (mainFrame) {
		mainFrame->Close(); // zatvori samo prozor
	}
	else {
		wxTheApp->ExitMainLoop(); // sigurno zatvara aplikaciju
	}
}

// Detect leftover files
std::vector<std::wstring> SearchLeftoverFiles(const std::vector<std::wstring>& paths, const std::string& programName) {
	std::vector<std::wstring> found;

	for (const auto& basePath : paths) {
		std::wstring fullPath = basePath + std::wstring(programName.begin(), programName.end());
		if (PathFileExistsW(fullPath.c_str())) {

			found.push_back(fullPath);
		}
	}

	return found;
}


// Detect leftover registry keys
 std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const std::string& programName) {
	std::vector<std::wstring> foundKeys;

	for (const auto& subKey : registryPaths) {
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			WCHAR keyName[256];
			DWORD keyNameLen = 256;
			DWORD index = 0;

			while (RegEnumKeyExW(hKey, index++, keyName, &keyNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
				std::wstring nameStr(keyName);
				keyNameLen = 256; // reset size

				std::string progNameStr(programName);
				std::wstring progNameW(progNameStr.begin(), progNameStr.end());

				if (nameStr.find(progNameW) != std::wstring::npos) {
					std::wstring fullKey = subKey + nameStr;
					foundKeys.push_back(fullKey);
					wxLogInfo("Found leftover registry key: %s", wxString(fullKey));
				}
			}

			RegCloseKey(hKey);
		}
		else {
			wxLogError("Registry path not found or inaccessible: %s", wxString(subKey));
		}
	}

	return foundKeys;
}


// Detect services and processes
std::vector<std::wstring> SearchServicesAndProcesses(const std::wstring& programName) {
	std::vector<std::wstring> foundServices;

	SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (hSCManager) {
		DWORD bytesNeeded = 0, serviceCount = 0, resumeHandle = 0;

		// Get required buffer size
		EnumServicesStatusExW(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE,
			NULL, 0, &bytesNeeded, &serviceCount, &resumeHandle, NULL);

		BYTE* buffer = new BYTE[bytesNeeded];
		if (EnumServicesStatusExW(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE,
			buffer, bytesNeeded, &bytesNeeded, &serviceCount, &resumeHandle, NULL)) {

			LPENUM_SERVICE_STATUS_PROCESS services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(buffer);

			for (DWORD i = 0; i < serviceCount; ++i) {
				std::wstring serviceName(services[i].lpServiceName);
				if (serviceName.find(programName) != std::wstring::npos) {
					foundServices.push_back(serviceName);
					wxLogInfo("Found leftover service: %s", wxString(serviceName));
				}
			}
		}
		else {
			wxLogError("Failed to enumerate services.");
		}

		delete[] buffer;
		CloseServiceHandle(hSCManager);
	}
	else {
		wxLogError("Failed to open service control manager.");
	}

	// You can also scan running processes here if needed

	return foundServices;
}


// Cleanup leftovers 
std::vector<std::wstring> CleanUpLeftovers(const std::string& programName, const std::string& regPath) {
	std::vector<std::wstring> leftovers;

	std::wstring localAppData = GetKnownFolderPath(FOLDERID_LocalAppData);
	std::wstring roamingAppData = GetKnownFolderPath(FOLDERID_RoamingAppData);
	std::wstring programData = GetKnownFolderPath(FOLDERID_ProgramData);
	std::wstring programFiles = GetProgramFilesDir();
	std::wstring programFilesX86 = GetProgramFilesX86Dir();

	std::vector<std::wstring> filePaths = {
		programFiles,
		programFilesX86,
		localAppData,
		roamingAppData,
		programData
	};

	std::vector<std::wstring> registryPaths = {
		L"SOFTWARE\\",
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
	};

	std::vector<std::wstring> fileLeftovers = SearchLeftoverFiles(filePaths, programName);
	leftovers.insert(leftovers.end(), fileLeftovers.begin(), fileLeftovers.end());

	std::vector<std::wstring> regLeftovers = SearchRegistryKeys(registryPaths, regPath);
	leftovers.insert(leftovers.end(), regLeftovers.begin(), regLeftovers.end());

	std::vector<std::wstring> serviceLeftovers = SearchServicesAndProcesses(std::wstring(programName.begin(), programName.end()));
	leftovers.insert(leftovers.end(), serviceLeftovers.begin(), serviceLeftovers.end());

	return leftovers;
}


