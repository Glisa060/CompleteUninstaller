#pragma once
#include "util.h"

ErrorDialog::ErrorDialog(wxWindow* parent, const wxString& message, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(300, 200)),
    m_message(message), m_title(title) {
    // You can add other initialization code here
}

void ErrorDialog::ShowErrorMessage() const{
    wxMessageBox(m_message, m_title, wxICON_ERROR | wxOK);
}


void GetInstalledPrograms(std::map<wxString, wxString>& programs) {
    std::vector<wxString> registryPaths = {
        R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall)",
        R"(SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall)"
    };

    for (const auto& registryPath : registryPaths) {
        wxLogMessage("Checking registry path: %s", registryPath);

        wxRegKey regKey(wxRegKey::HKLM, registryPath); // Root key and path

        if (!regKey.Exists()) {
            wxLogMessage("Registry path does not exist: %s", registryPath);
            continue;
        }

        if (!regKey.Open(wxRegKey::Read)) {
            wxLogMessage("Failed to open registry path: %s", registryPath);
            continue;
        }

        wxLogMessage("Successfully opened registry path: %s", registryPath);

        wxString subKeyName;
        long index = 0;

        // Enumerate subkeys under the registry path
        if (regKey.GetFirstKey(subKeyName, index)) {
            wxLogMessage("Enumerating subkeys for: %s", registryPath);

            do {
                wxRegKey subKey(regKey, subKeyName);
                wxLogMessage("Checking subkey: %s", subKeyName);

                if (!subKey.Exists()) {
                    wxLogError("Subkey does not exist: %s", subKeyName);
                    continue;
                }

                if (!subKey.Open(wxRegKey::Read)) {
                    wxLogError("Failed to open subkey: %s", subKeyName);
                    continue;
                }

                wxString displayName, uninstallStr;

                // Read "DisplayName" value
                if (subKey.HasValue("DisplayName")) {
                    subKey.QueryValue("DisplayName", displayName);
                    wxLogMessage("Found DisplayName: %s", displayName);
                }
                else {
                    wxLogError("No DisplayName value in subkey: %s", subKeyName);
                }

                // Read "UninstallString" value
                if (subKey.HasValue("UninstallString")) {
                    subKey.QueryValue("UninstallString", uninstallStr);
                    wxLogMessage("Found UninstallString: %s", uninstallStr);
                }
                else {
                    wxLogError("No UninstallString value in subkey: %s", subKeyName);
                }

                // Only add to the map if both values are present
                if (!displayName.IsEmpty() && !uninstallStr.IsEmpty()) {
                    programs[displayName] = uninstallStr;
                    wxLogMessage("Added program: %s -> %s", displayName, uninstallStr);
                }
                else {
                    wxLogWarning("Skipping subkey due to missing values: %s", subKeyName);
                }
            } while (regKey.GetNextKey(subKeyName, index));
        }
        else {
            wxLogError("No subkeys found in registry path: %s", registryPath);
        }
    }

    wxLogMessage("Completed GetInstalledPrograms. Total programs found: %d", programs.size());
}


void GetUserInstalledPrograms(std::map<wxString, wxString>& programs) {
    wxRegKey regKey(wxRegKey::HKCU, R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall)");

    if (regKey.Exists() && regKey.Open(wxRegKey::Read)) {
        wxString subKeyName;
        long index = 0;

        // Log that we're starting to enumerate subkeys
        wxLogMessage("Enumerating installed programs...");

        // Enumerate all subkeys
        if (regKey.GetFirstKey(subKeyName, index)) {
            do {
                wxRegKey subKey(regKey, subKeyName);

                if (subKey.Exists() && subKey.Open(wxRegKey::Read)) {
                    wxString displayName, uninstallStr;

                    // Log the current subkey being processed
                    wxLogMessage("Processing subkey: %s", subKeyName);

                    // Read "DisplayName" value
                    if (subKey.HasValue("DisplayName")) {
                        subKey.QueryValue("DisplayName", displayName);
                    }
                    else {
                        // Log if "DisplayName" is missing
                        wxLogWarning("Missing 'DisplayName' for subkey: %s", subKeyName);
                    }

                    // Read "UninstallString" value
                    if (subKey.HasValue("UninstallString")) {
                        subKey.QueryValue("UninstallString", uninstallStr);
                    }
                    else {
                        // Log if "UninstallString" is missing
                        wxLogWarning("Missing 'UninstallString' for subkey: %s", subKeyName);
                    }

                    // Only add to the map if both values are present
                    if (!displayName.IsEmpty() && !uninstallStr.IsEmpty()) {
                        programs[displayName] = uninstallStr;

                        // Log the program being added to the map
                        wxLogMessage("Added program: %s", displayName);
                    }
                }
                else {
                    // Log if the subkey could not be opened
                    wxLogError("Failed to open subkey: %s", subKeyName);
                }

            } while (regKey.GetNextKey(subKeyName, index));
        }
        else {
            // Log if no subkeys were found
            wxLogWarning("No subkeys found in the registry for installed programs.");
        }
    }
    else {
        // Log if the registry key could not be opened
        wxLogError("Failed to open registry key for installed programs.");
    }
    wxLogMessage("Completed GetUserInstalledPrograms. Total programs found: %d", programs.size());

}