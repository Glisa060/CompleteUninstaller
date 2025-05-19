#pragma once
#include <string>
#include <vector>
#include <shtypes.h>

std::vector<std::wstring> SearchLeftoverFiles(const std::vector<std::wstring>& paths, const std::string& programName);
std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId);
std::wstring GetProgramFilesDir();
std::wstring GetProgramFilesX86Dir();
bool DeleteFileOrFolder(const std::wstring& path);


