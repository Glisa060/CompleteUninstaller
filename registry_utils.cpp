#include <wx/log.h>
#include <wx/string.h>
#include <wx/busyinfo.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <winreg.h>
#include "registry_utils.h"

// Brisanje registry kljuca i svih podkljuceva (rekurzivno)
LONG RegDeleteKeyRecursive(HKEY hKeyRoot, const std::wstring& subKey) {
	HKEY hKey;
	LONG lRes = RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey);
	if (lRes != ERROR_SUCCESS) {
		return lRes;
	}

	WCHAR name[256];
	DWORD size = 256;

	while (RegEnumKeyExW(hKey, 0, name, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
		std::wstring sub(subKey + L"\\" + name);
		RegDeleteKeyRecursive(hKeyRoot, sub);
		size = 256;  // reset buffer size
	}

	RegCloseKey(hKey);
	return RegDeleteKeyW(hKeyRoot, subKey.c_str());
}

// Pretraga registry kljuceva koji sadrže ime programa
std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const wxString& programName) {
	wxBusyInfo info("Searching registry keys...");
	std::vector<std::wstring> foundKeys;

	std::wstring progNameW(programName.begin(), programName.end());

	for (const auto& subKey : registryPaths) {
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
			wxLogWarning("Registry path inaccessible or not found: %s", wxString(subKey));
			continue;
		}

		DWORD index = 0;
		WCHAR keyName[256];

		while (true) {
			DWORD keyNameLen = 256;  // mora da se resetuje svaki put
			LONG result = RegEnumKeyExW(hKey, index++, keyName, &keyNameLen, NULL, NULL, NULL, NULL);
			if (result == ERROR_NO_MORE_ITEMS) break;
			if (result != ERROR_SUCCESS) continue;

			std::wstring nameStr(keyName);
			if (nameStr.find(progNameW) != std::wstring::npos) {
				std::wstring fullKey = subKey + L"\\" + nameStr;
				foundKeys.push_back(fullKey);
				wxLogInfo("Found leftover registry key: %s", wxString(fullKey));
			}
		}

		RegCloseKey(hKey);
	}

	return foundKeys;
}

