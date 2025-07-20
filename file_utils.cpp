
#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/busyinfo.h>
#include <wx/log.h>
#include <wx/string.h>
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <windows.h>
#include <vector>
#include <exception>
#include <filesystem>
#include <unordered_set>
#include <regex>
#include <shlwapi.h>
#include <ShlObj_core.h>

#pragma comment(lib, "Shlwapi.lib")
#include "file_utils.h"

namespace fs = std::filesystem;
fs::path lastValidPath;

std::vector<std::wstring> SearchLeftoverFiles(
    const std::vector<std::wstring>& directories,
    const std::vector<std::wstring>& searchTerms
) {
    std::vector<std::wstring> foundFiles;
    const std::wstring lowerTerm = ToLower(searchTerms.front());
    wxLogMessage("Search term all lower caps: %s", lowerTerm);

    std::unordered_set<std::wstring> ignoredFolders = {
        L"node_modules", L"npm", L"yarn", L"cache", L"temp", L"packages"
    };

    for (const auto& dir : directories) {
        fs::path root(dir);
        if (!fs::exists(root) || !fs::is_directory(root)) {
            wxLogWarning("Directory does not exist or is not a directory: %s", root.wstring());
            continue;
        }

        for (const auto& entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied)) {
            try {
                if (!entry.is_directory()) continue;

                fs::path subPath = entry.path();
                std::wstring folderName = ToLower(subPath.filename().wstring());
                std::wstring subPathStr = ToLower(subPath.wstring());

                bool forceDeepTraversal = IsDeepTraversalPath(subPathStr);

                // Skip noisy folders unless they match the search term
                if (!forceDeepTraversal &&
                    ignoredFolders.contains(folderName) &&
                    folderName.find(lowerTerm) == std::wstring::npos) {
                    wxLogMessage("Skipping noisy unrelated folder: %s", subPath.wstring());
                    continue;
                }

                // Match top-level folder directly
                if (subPathStr.find(lowerTerm) != std::wstring::npos) {
                    wxLogMessage("Matched top-level folder: %s", subPath.wstring());
                    foundFiles.push_back(subPath.wstring());
                }

                // Traverse deeply into allowed subdirectory
                wxLogMessage("Traversing: %s", subPath.wstring());
                fs::recursive_directory_iterator it(subPath, fs::directory_options::skip_permission_denied), end;
                fs::path lastValidPath;

                while (it != end) {
                    try {
                        lastValidPath = it->path();
                        std::wstring current = ToLower(lastValidPath.wstring());

                        if (current.find(lowerTerm) != std::wstring::npos) {
                            wxLogMessage("Matched leftover: %s", lastValidPath.wstring());
                            foundFiles.push_back(lastValidPath.wstring());
                        }

                        ++it;
                    }
                    catch (const std::exception& e) {
                        wxLogError("Error incrementing iterator after: %s\nReason: %s",
                            lastValidPath.wstring(), e.what());
                        break;
                    }
                }
            }
            catch (const std::exception& e) {
                wxLogError("Error accessing subdir: %s", e.what());
                continue;
            }
        }
    }

    return foundFiles;
}

std::wstring ToLower(const std::wstring& str) {
    std::wstring lower;
    std::transform(str.begin(), str.end(), std::back_inserter(lower), ::towlower);
    return lower;
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

bool DeleteFileOrFolder(const std::wstring& path) {
	if (PathIsDirectoryW(path.c_str())) {
		// Folder - use recursive delete
		WIN32_FIND_DATAW findFileData;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		std::wstring spec = path + L"\\*";

		hFind = FindFirstFileW(spec.c_str(), &findFileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			// Folder is empty or doesn't exist
			return RemoveDirectoryW(path.c_str()) == TRUE;
		}

		do {
			const std::wstring fileOrDir = findFileData.cFileName;
			if (fileOrDir == L"." || fileOrDir == L"..")
				continue;

			std::wstring fullSubPath = path + L"\\" + fileOrDir;

			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteFileOrFolder(fullSubPath);  // use recursive delete for subdirectory
			}
			else {
				DeleteFileW(fullSubPath.c_str());
			}
		} while (FindNextFileW(hFind, &findFileData) != 0);

		FindClose(hFind);
		return RemoveDirectoryW(path.c_str()) == TRUE;
	}
	else {
		// File - delete directly
		return DeleteFileW(path.c_str()) == TRUE;
	}
}

bool IsDeepTraversalPath(const std::wstring& path) {
    std::wstring lower = ToLower(path);
    return lower.find(L"appdata\\local\\programs") != std::wstring::npos ||
        lower.find(L"appdata\\local") != std::wstring::npos ||
        lower.find(L"appdata\\roaming") != std::wstring::npos ||
        lower.find(L"appdata\\locallow") != std::wstring::npos ||
        lower.find(L"programdata") != std::wstring::npos;
}

std::wstring NormalizeSearchTerm(const std::wstring& term) {
    std::wregex versionPattern(LR"(\s*\d+(\.\d+)*$)");
    return std::regex_replace(term, versionPattern, L"");
}


