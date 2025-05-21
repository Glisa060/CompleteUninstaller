
#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/busyinfo.h>
#include <wx/log.h>
#include <wx/string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <exception>
#include <filesystem>
#include <set>
#include <shlwapi.h>
#include <ShlObj_core.h>

#pragma comment(lib, "Shlwapi.lib")
#include "file_utils.h"

namespace fs = std::filesystem;

// Detect leftover files
std::vector<std::wstring> SearchLeftoverFiles(const std::vector<std::wstring>& paths, const std::string& programName) {
    wxBusyInfo* info = new wxBusyInfo("Searching for leftover files...");
    std::vector<std::wstring> found;
    std::wstring progNameW(programName.begin(), programName.end());
    bool permissionErrorShown = false;

    static const std::set<std::wstring> skipFolders = {
        L"System Volume Information",
        L"$Recycle.Bin",
        L"Config.Msi",
        L"MSOCache",
        L"Recovery",
        L"WindowsApps",
        L"ProgramData\\Microsoft\\Windows Defender"
    };

    wxLogMessage("SearchLeftoverFiles started for program: %s", wxString(programName));

    for (const auto& basePath : paths) {
        wxLogMessage("Scanning base path: %s", wxString(basePath));

        try {
            if (!fs::exists(basePath)) {
                wxLogWarning("Path does not exist: %s", wxString(basePath));
                continue;
            }
            if (!fs::is_directory(basePath)) {
                wxLogWarning("Not a directory: %s", wxString(basePath));
                continue;
            }

            for (const auto& entry : fs::recursive_directory_iterator(basePath, fs::directory_options::skip_permission_denied)) {
                try {
                    const auto& entryPath = entry.path();
                    const auto relPath = entryPath.lexically_relative(basePath).wstring();

                    bool skip = false;
                    for (const auto& folder : skipFolders) {
                        if (relPath.find(folder) != std::wstring::npos || entryPath.filename().wstring() == folder) {
                            wxLogDebug("Skipping known system folder: %s", wxString(entryPath.wstring()));
                            skip = true;
                            break;
                        }
                    }
                    if (skip) continue;

                    if (entry.is_directory()) {
                        std::wstring folderName = entryPath.filename().wstring();
                        wxLogDebug("Checking folder: %s", wxString(entryPath.wstring()));

                        if (folderName.find(progNameW) != std::wstring::npos) {
                            found.push_back(entryPath.wstring());
                            wxLogInfo("Matched folder: %s", wxString(entryPath.wstring()));
                        }
                    }
                    else if (entry.is_regular_file()) {
                        std::wstring fileName = entryPath.filename().wstring();
                        wxLogDebug("Checking file: %s", wxString(entryPath.wstring()));

                        if (fileName.find(progNameW) != std::wstring::npos) {
                            found.push_back(entryPath.wstring());
                            wxLogInfo("Matched file: %s", wxString(entryPath.wstring()));
                        }
                    }
                }
                catch (const std::exception& inner) {
                    wxLogError("Error processing entry: %s | %s", wxString(entry.path().wstring()), inner.what());
                }
            }
        }
        catch (const std::exception& e) {
            wxLogError("Exception while processing base path: %s | %s", wxString(basePath), e.what());
            if (!permissionErrorShown) {
                permissionErrorShown = true;
                wxTheApp->CallAfter([] {
                    wxMessageBox(
                        "Some folders could not be searched due to permission issues.\n"
                        "Run the application as administrator.",
                        "Permission Denied",
                        wxICON_WARNING | wxOK
                    );
                });
            }
        }
    }

    wxLogMessage("SearchLeftoverFiles complete. %zu items found.", found.size());

    // Must show messagebox in main thread
    wxTheApp->CallAfter([info]() {
        delete info;
        wxMessageBox("File search completed.\nCheck logs for details.", "Search Complete", wxOK | wxICON_INFORMATION);
    });

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
