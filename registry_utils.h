#pragma once

#include <wx/string.h>
#include <windows.h>
#include <string>
#include <vector>
#include <map>

extern std::map<wxString, wxString> registryPaths;

// Recursively deletes the given registry key and all its subkeys.
// fullKeyPath can start with "HKLM\" or "HKCU\" etc.
LONG RegDeleteKeyRecursiveByPath(const std::wstring& fullKeyPath);
LONG RegDeleteKeyRecursiveByPath(const std::string& fullKeyPath);
LONG RegDeleteKeyRecursiveByPath(const wxString& fullKeyPath);

// Searches registry keys in the given relative paths under both
// HKEY_LOCAL_MACHINE and HKEY_CURRENT_USER for a given program name (case insensitive).
// Returns a vector of full key paths found.
std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const wxString& programName);
void ReadProgramsFromRegistry(HKEY root, const std::string& path,
    std::map<wxString, wxString>& programs,
    std::map<wxString, wxString>& manufacturers);
std::string GetRegistryPathForProgram(const std::string& name);
void GetInstalledPrograms(std::map<wxString, wxString>& programs,
    std::map<wxString, wxString>& manufacturers);