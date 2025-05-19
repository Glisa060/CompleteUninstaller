#pragma once

#include <wx/string.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>


LONG RegDeleteKeyRecursive(HKEY hKeyRoot, const std::wstring& subKey);
std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const wxString& programName); 


