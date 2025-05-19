#include <wx/app.h>
#include <wx/busyinfo.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/frame.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <map>
#include <string>
#include <thread>
#include <functional>
#include <winreg.h>
#include <shellapi.h>
#include <errhandlingapi.h>
#include <KnownFolders.h>
#include <libloaderapi.h>
#include <ShlObj_core.h>
#include <algorithm>
#include <cctype>
#include <vector>
#include <filesystem>
#include "file_utils.h"
#include "process_utils.h"
#include "registry_utils.h"
#include "util.h"


namespace fs = std::filesystem;

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



void runAsAdmin(wxFrame* mainFrame) {
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

// Detect leftover registry keys
// Pomocna funkcija za poredjenje stringova bez obzira na velika/mala slova
bool ContainsIgnoreCase(const std::wstring& haystack, const std::wstring& needle) {
	std::wstring h = haystack;
	std::wstring n = needle;
	std::transform(h.begin(), h.end(), h.begin(), ::towlower);
	std::transform(n.begin(), n.end(), n.begin(), ::towlower);
	return h.find(n) != std::wstring::npos;
}




// Cleanup leftovers 
void CleanUpLeftovers(const std::string& programName, const wxString& regPath, std::function<void()> onFinished)
{
	wxBusyInfo* busy = new wxBusyInfo("Cleaning leftovers, please wait...");

	std::thread([programName, regPath, onFinished, busy]() {
		wxLogMessage("Starting cleanup for program: %s", programName);
		wxLogMessage("Registry path: %s", regPath);

		std::wstring localAppData = GetKnownFolderPath(FOLDERID_LocalAppData);
		wxLogMessage("LocalAppData folder: %s", wxString(localAppData));

		std::wstring roamingAppData = GetKnownFolderPath(FOLDERID_RoamingAppData);
		wxLogMessage("RoamingAppData folder: %s", wxString(roamingAppData));

		std::wstring programData = GetKnownFolderPath(FOLDERID_ProgramData);
		wxLogMessage("ProgramData folder: %s", wxString(programData));

		std::wstring programFiles = GetProgramFilesDir();
		wxLogMessage("ProgramFiles folder: %s", wxString(programFiles));

		std::wstring programFilesX86 = GetProgramFilesX86Dir();
		wxLogMessage("ProgramFilesX86 folder: %s", wxString(programFilesX86));

		std::vector<std::wstring> filePaths = {
			programFiles,
			programFilesX86,
			localAppData,
			roamingAppData,
			programData
		};

		std::vector<std::wstring> registrySearchPaths = {
			L"SOFTWARE\\",
			L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
		};

		// Pretraga fajlova za brisanje
		auto fileLeftovers = SearchLeftoverFiles(filePaths, programName);

		for (const auto& filePath : fileLeftovers) {
			wxTheApp->CallAfter([filePath]() {
				wxLogMessage("Deleting leftover file/folder: %s", wxString(filePath));
			});
			DeleteFileOrFolder(filePath);
		}

		// Pretraga registry kljuceva za brisanje
		auto regLeftovers = SearchRegistryKeys(registrySearchPaths, regPath);

		for (const auto& regKey : regLeftovers) {
			wxTheApp->CallAfter([regKey]() {
				wxLogMessage("Deleting leftover registry key: %s", wxString(regKey));
			});
			RegDeleteKeyRecursive(HKEY_LOCAL_MACHINE, regKey);
		}

		// Pretraga i brisanje servisa/procesa
		auto serviceLeftovers = SearchServicesAndProcesses(std::wstring(programName.begin(), programName.end()));

		for (const auto& serviceName : serviceLeftovers) {
			wxTheApp->CallAfter([serviceName]() {
				wxLogMessage("Stopping and deleting leftover service/process: %s", wxString(serviceName));
			});
			StopAndDeleteService(serviceName);
		}

		// Uništi wxBusyInfo u glavnom thread-u
		wxTheApp->CallAfter([busy]() {
			delete busy;
		});

		// Pozovi callback da javi da je gotovo
		wxTheApp->CallAfter([onFinished]() {
			onFinished();
		});

	}).detach();
}