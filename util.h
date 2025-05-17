#pragma once

#include <wx/treectrl.h>
#include <wx/wx.h>
#include <map>
#include <wx/log.h>
#include <wx/string.h>
#include <winreg.h>
#include <wx/msgdlg.h>
#include <shlwapi.h> // Add this include for PathFileExistsW
#pragma comment(lib, "Shlwapi.lib") // Link with Shlwapi.lib to resolve PathFileExistsW
#include <shlobj.h>     // SHGetKnownFolderPath
#include <knownfolders.h>
#include <format>

class ErrorDialog : public wxDialog {
public:
	ErrorDialog(wxWindow* parent, const wxString& message, const wxString& title);

	void ShowErrorMessage() const;

private:
	wxString m_message;
	wxString m_title;
};

void GetInstalledPrograms(std::map<wxString, wxString>& programs);
void runAsAdmin(wxFrame* mainFrame);

std::vector<std::wstring> SearchLeftoverFiles(const std::vector<std::wstring>& paths, const std::string& programName);
std::vector<std::wstring> SearchRegistryKeys(const std::vector<std::wstring>& registryPaths, const std::string& programName);
std::vector<std::wstring> SearchServicesAndProcesses(const std::wstring& programName);
std::vector<std::wstring> CleanUpLeftovers(const std::string& programName, const std::string& regPath);
std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId);
std::wstring GetProgramFilesDir();
std::wstring GetProgramFilesX86Dir();
