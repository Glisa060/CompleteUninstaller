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




