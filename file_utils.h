#pragma once
#include <string>
#include <vector>

struct AnalysisResult {
	std::vector<std::wstring> files;
	std::vector<std::wstring> registryKeys;
	std::vector<std::wstring> services;
};

std::vector<std::wstring> SearchLeftoverFiles(const std::vector<std::wstring>& directories, const std::vector<std::wstring>& searchTerms);
std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId);
std::wstring GetProgramFilesDir();
std::wstring GetProgramFilesX86Dir();
bool DeleteFileOrFolder(const std::wstring& path);
std::wstring ToLower(const std::wstring& str);
bool IsDeepTraversalPath(const std::wstring& path);
std::wstring NormalizeSearchTerm(const std::wstring& term);

