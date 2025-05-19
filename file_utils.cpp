
#include <wx/busyinfo.h>
#include <wx/log.h>
#include <wx/string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <exception>
#include <filesystem>
#include <shlwapi.h>
#include <ShlObj_core.h>



#pragma comment(lib, "Shlwapi.lib")
#include "file_utils.h"

namespace fs = std::filesystem;


// Detect leftover files
std::vector<std::wstring> SearchLeftoverFiles(const std::vector<std::wstring>& paths, const std::string& programName) {
	wxBusyInfo info("Searching for leftover files...");
	std::vector<std::wstring> found;
	std::wstring progNameW(programName.begin(), programName.end());

	for (const auto& basePath : paths) {
		try {
			if (!fs::exists(basePath) || !fs::is_directory(basePath))
				continue;

			for (const auto& entry : fs::recursive_directory_iterator(basePath)) {
				if (entry.is_directory()) {
					std::wstring folderName = entry.path().filename().wstring();
					if (folderName.find(progNameW) != std::wstring::npos) {
						found.push_back(entry.path().wstring());
						wxLogInfo("Found folder: %s", wxString(entry.path().wstring()));
					}
				}
				if (entry.is_regular_file()) {
					std::wstring fileName = entry.path().filename().wstring();
					if (fileName.find(progNameW) != std::wstring::npos) {
						found.push_back(entry.path().wstring());
						wxLogInfo("Found file: %s", wxString(entry.path().wstring()));
					}
				}
			}
		}
		catch (const std::exception& e) {
			wxLogError("Failed to search in path: %s | %s", wxString(basePath), e.what());
		}
	}

	return found;
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
		// Folder - rekurzivno obriši sadržaj
		WIN32_FIND_DATAW findFileData;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		std::wstring spec = path + L"\\*";

		hFind = FindFirstFileW(spec.c_str(), &findFileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			// Folder je prazan ili greška
			return RemoveDirectoryW(path.c_str()) == TRUE;
		}

		do {
			const std::wstring fileOrDir = findFileData.cFileName;
			if (fileOrDir == L"." || fileOrDir == L"..")
				continue;

			std::wstring fullSubPath = path + L"\\" + fileOrDir;

			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteFileOrFolder(fullSubPath);  // rekurzivno obriši folder
			}
			else {
				DeleteFileW(fullSubPath.c_str());
			}
		} while (FindNextFileW(hFind, &findFileData) != 0);

		FindClose(hFind);
		return RemoveDirectoryW(path.c_str()) == TRUE;
	}
	else {
		// Fajl
		return DeleteFileW(path.c_str()) == TRUE;
	}
}
