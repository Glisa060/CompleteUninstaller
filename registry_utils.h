#pragma once

#include <string>
#include <vector>
#include <wx/string.h>
#include <windows.h>

// Recursively deletes the given registry key and all its subkeys.
// fullKeyPath can start with "HKLM\" or "HKCU\" etc.
LONG RegDeleteKeyRecursiveByPath(const std::wstring& fullKeyPath);
LONG RegDeleteKeyRecursiveByPath(const std::string& fullKeyPath);
LONG RegDeleteKeyRecursiveByPath(const wxString& fullKeyPath);

// Searches registry keys in the given relative paths under both
// HKEY_LOCAL_MACHINE and HKEY_CURRENT_USER for a given program name (case insensitive).
// Returns a vector of full key paths found.
std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const wxString& programName);
