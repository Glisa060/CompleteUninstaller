#include <wx/app.h>
#include <wx/busyinfo.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/treectrl.h>
#include <wx/sysopt.h> // Add this include to resolve wxSystemOptions
#include <wx/config.h>
#include <wx/fileconf.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include "ThreadedSearchHelper.h"
#include "file_utils.h"
#include "process_utils.h"
#include "registry_utils.h"
#include "gui.h"
#include "util.h"
#include "admin_utils.h"
#include "enums.h"


wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(Minimal_Quit, MyFrame::OnQuit)
EVT_MENU(Minimal_About, MyFrame::OnAbout)
EVT_MENU(Minimal_Open, MyFrame::OnOpen)
EVT_MENU(Minimal_Analyse, MyFrame::OnAnalyseMenu)
EVT_MENU(RestartAsAdmin_ID, MyFrame::OnRestartAsAdmin)
EVT_MENU(Theme_Light, MyFrame::OnThemeSelect)
EVT_MENU(Theme_Dark, MyFrame::OnThemeSelect)
EVT_TREE_SEL_CHANGED(Run_Selected, MyFrame::OnTreeSelectionChanged)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);

std::map<wxString, wxString> installedPrograms;
std::map<wxString, wxString> uninstallerPaths;
std::map<wxString, wxString> registryPaths;
std::string selectedUninstallerPath;

// === App lifecycle ===

bool MyApp::OnInit() {
	if (!wxApp::OnInit())
		return false;

	logFile = fopen("logfile.txt", "w");
	if (!logFile) {
		wxLogError("Failed to open log file!");
		return false;
	}

	wxLog::SetActiveTarget(new wxLogStderr(logFile, wxConvUTF8));

	if (!IsRunningAsAdmin()) {
		wxLogWarning("Not running as administrator.");
		if (RestartAsAdmin()) {
			wxLogMessage("Launched elevated instance. Exiting current instance.");
			return false; // Terminate this instance
		}
		else {
			wxMessageBox("The application must be run as administrator.", "Error", wxICON_ERROR);
			return false;
		}
	}

	wxLogMessage("OnInit method Log: App started running as administrator.");

	auto* frame = new MyFrame("Complete Uninstaller 0.1 alpha");
	frame->Show(true);
	wxSystemOptions::SetOption("msw.dark-mode", 1);
	return true;
}

int MyApp::OnExit() {
	if (logFile) {
		fclose(logFile);
		logFile = nullptr;
	}
	return 0;
}

// === Event handlers ===

void MyFrame::OnQuit(wxCommandEvent&) {
	Close(true);
}

void MyFrame::OnAbout(wxCommandEvent&) {
	wxMessageBox(wxString::Format(
		"Complete Uninstaller 0.1 alpha build\n"
		"For Windows\n"
		"Using wxWidgets %s\n%s",
		wxGetLibraryVersionInfo().GetVersionString(),
		wxGetOsDescription()),
		"About", wxOK | wxICON_INFORMATION, this);
}

void MyFrame::OnOpen(wxCommandEvent&) {
	auto selected = treeCtrl->GetSelection();
	if (!selected.IsOk()) {
		wxMessageBox("Select a program to uninstall.", "No Selection", wxOK | wxICON_WARNING, this);
		return;
	}

	auto programName = treeCtrl->GetItemText(selected).ToStdString();
	auto it = uninstallerPaths.find(programName);
	if (it == uninstallerPaths.end()) {
		wxMessageBox("Uninstaller path not found.", "Error", wxICON_ERROR);
		return;
	}

	wxString command = wxString(it->second).Trim(true).Trim(false);
	if (!command.StartsWith("MsiExec.exe") && !wxFileExists(command)) {
		wxMessageBox("Invalid or missing uninstaller file.", "Error", wxICON_ERROR);
		return;
	}

	long result = wxExecute(command, wxEXEC_ASYNC);
	if (result == -1) {
		wxMessageBox("Failed to launch uninstaller.", "Error", wxOK | wxICON_ERROR);
		return;
	}

	wxMessageBox("Uninstaller launched.", "Success", wxOK | wxICON_INFORMATION);
	OnProgramListUpdated();

	wxString folderName = wxFileName(command).GetDirs().Last();
	CleanUpLeftovers(folderName.ToStdString(), GetRegistryPathForProgram(folderName.ToStdString()), [this]() {
		wxMessageBox("Cleanup finished.", "Done", wxICON_INFORMATION, this);
	});
}

void MyFrame::OnTreeSelectionChanged(wxTreeEvent& event) {
	auto selected = event.GetItem();
	if (!selected.IsOk()) return;

	wxString name = treeCtrl->GetItemText(selected);
	auto it = uninstallerPaths.find(name.ToStdString());

	if (it != uninstallerPaths.end()) {
		selectedUninstallerPath = it->second;
		SetStatusText(wxString::Format("Uninstaller: %s", selectedUninstallerPath), 1);
	}
	else {
		SetStatusText("Uninstaller: <not found>", 1);
		selectedUninstallerPath.clear();
	}
}

void MyFrame::OnAnalyseMenu(wxCommandEvent&)
{
	if (!IsRunningAsAdmin()) {
		int res = wxMessageBox(
			"This action requires administrator rights to scan all folders and registry entries.\n\n"
			"Do you want to restart the application with elevated privileges?",
			"Administrator Access Required",
			wxICON_QUESTION | wxYES_NO
		);

		if (res == wxYES) {
			if (RestartAsAdmin()) {
				Close(); // Optional: close current instance
			}
		}

		return;
	}

	auto selected = treeCtrl->GetSelection();
	if (!selected.IsOk()) {
		wxMessageBox("Select a program to analyse.", "No Selection", wxICON_INFORMATION);
		return;
	}

	auto name = treeCtrl->GetItemText(selected).ToStdString();

	// Get uninstall string from map (or registry)
	wxString uninstallStr = uninstallerPaths.contains(name) ? uninstallerPaths[name] : wxString("Uninstall string not found.");

	std::vector<std::wstring> regVec = { std::wstring(registryPaths[name].begin(), registryPaths[name].end()) };

	// Create wxBusyInfo on the heap, so it lives beyond this function scope
	wxBusyInfo* busy = new wxBusyInfo("Analysing leftovers...", this);

	RunInBackground<AnalysisResult>(
		[name, regVec]() -> AnalysisResult {
		try {
			std::vector<std::wstring> filePaths = {
				GetProgramFilesDir(), GetProgramFilesX86Dir(),
				GetKnownFolderPath(FOLDERID_LocalAppData),
				GetKnownFolderPath(FOLDERID_RoamingAppData),
				GetKnownFolderPath(FOLDERID_ProgramData)
			};

			std::vector<std::wstring> searchTerms = { std::wstring(name.begin(), name.end()) };

			auto files = SearchLeftoverFiles(filePaths, searchTerms);
			auto reg = SearchRegistryKeys(regVec, std::wstring(name.begin(), name.end()));
			auto svc = SearchServicesAndProcesses(std::wstring(name.begin(), name.end()));

			return { files, reg, svc };
		}
		catch (const std::exception& e) {
			wxLogError("Exception in background search: %s", e.what());
			wxTheApp->CallAfter([]() {
				wxLogError("Error during background search.\nCheck logs for details.");
			});
			return {};
		}
	},
		[this, uninstallStr, busy](AnalysisResult result) {
		wxTheApp->CallAfter([this, result, uninstallStr, busy]() {
			delete busy; // Close the busy popup

			wxLogMessage("Uninstall string: %s", uninstallStr);

			size_t total = result.files.size() + result.registryKeys.size() + result.services.size();

			if (total == 0) {
				wxMessageBox("No leftovers found.", "Clean", wxICON_INFORMATION);
			}
			else {
				wxString msg;
				msg.Printf("Leftovers found:\n\n"
					"- Files: %zu\n"
					"- Registry Keys: %zu\n"
					"- Services/Processes: %zu",
					result.files.size(),
					result.registryKeys.size(),
					result.services.size());
				wxMessageBox(msg, "Analysis Results", wxOK | wxICON_INFORMATION);

				// You can now use individual vectors for display
				DisplayLeftovers(result.files, result.registryKeys, result.services);
			}
		});
	}

	);
}

// === UI helpers ===

void MyFrame::PopulateTreeView() {
	treeCtrl->DeleteAllItems();

	auto root = treeCtrl->AddRoot("Installed Programs", 0);  // Ikonica ID 0
	auto userNode = treeCtrl->AppendItem(root, "[User] Installed", 1);    // Ikonica ID 1
	auto sys32Node = treeCtrl->AppendItem(root, "[32-bit] System", 1);   // Ikonica ID 2
	auto sys64Node = treeCtrl->AppendItem(root, "[64-bit] System", 1);   // Ikonica ID 3

	for (const auto& [name, regPath] : installedPrograms) {
		std::string key = regPath.ToStdString();
		wxTreeItemId* node = &sys64Node;

		if (key.find("WOW6432Node") != std::string::npos)
			node = &sys32Node;
		else if (key.find("HKEY_CURRENT_USER") != std::string::npos || key.find("HKCU") != std::string::npos)
			node = &userNode;
		else if (key.find("CurrentUser") != std::string::npos)
			node = &userNode;

		// Dodaj ikonicu programa (ID 4)
		treeCtrl->AppendItem(*node, name, 2);
		uninstallerPaths[name.ToStdString()] = regPath.ToStdString();
	}

	treeCtrl->ExpandAll();
}

void MyFrame::OnProgramListUpdated() {
	auto* info = new wxBusyInfo("Loading installed programs...");

	std::thread([this, info]() {
		std::map<wxString, wxString> programs;
		std::map<wxString, wxString> mfgs;
		GetInstalledPrograms(programs, mfgs);

		wxTheApp->CallAfter([this, programs = std::move(programs), mfgs = std::move(mfgs), info]() {
			delete info;
			installedPrograms = std::move(programs);
			manufacturers = std::move(mfgs);
			PopulateTreeView();
		});
	}).detach();
}

void MyFrame::DisplayProgramDetails(const std::vector<std::wstring>& leftoverItems)
{
	leftovers = leftoverItems;

	leftoversListCtrl->DeleteAllItems();
	for (size_t i = 0; i < leftoverItems.size(); ++i) {
		wxString item(leftoverItems[i].c_str());
		leftoversListCtrl->InsertItem(i, item);
	}
}

void MyFrame::OnDeleteSelected(wxCommandEvent&)
{
	long itemIndex = -1;
	std::vector<std::wstring> toDelete;

	// Collect selected leftovers to delete
	while ((itemIndex = leftoversListCtrl->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND) {
		wxString wxItemText = leftoversListCtrl->GetItemText(itemIndex);
		std::wstring itemText(wxItemText.begin(), wxItemText.end());
		toDelete.push_back(itemText);
	}

	if (toDelete.empty()) {
		wxMessageBox("No leftovers selected for deletion.", "Info", wxICON_INFORMATION);
		return;
	}

	if (wxMessageBox("Are you sure you want to delete the selected leftovers?", "Confirm Deletion", wxYES_NO | wxICON_QUESTION) != wxYES) {
		return;
	}

	for (const auto& leftover : toDelete) {
		// Determine if leftover is a registry key or a file path, then delete accordingly
		if (leftover.find(L"HKLM\\") == 0 || leftover.find(L"HKCU\\") == 0) {
			// It's a registry key - delete recursively
			LONG res = RegDeleteKeyRecursiveByPath(leftover);
			if (res == ERROR_SUCCESS) {
				wxLogMessage("Deleted registry key: %s", wxString(leftover));
			}
			else {
				wxLogWarning("Failed to delete registry key: %s", wxString(leftover));
			}
		}
		else {
			// Assume it's a file or folder path
			if (DeleteFileOrFolder(leftover)) {
				wxLogMessage("Deleted file/folder: %s", wxString(leftover));
			}
			else {
				wxLogWarning("Failed to delete file/folder: %s", wxString(leftover));
			}
		}
	}

	// Refresh leftover list to remove deleted items
	wxMessageBox("Deletion complete. Refreshing leftovers list...", "Info", wxICON_INFORMATION);

	// You might want to rerun analysis here or remove items from the UI
	// For now, just clear selected items:
	//DisplayLeftovers({});  // Clear list
}

void MyFrame::OnRestartAsAdmin(wxCommandEvent&) {
	if (!IsRunningAsAdmin()) {
		if (RestartAsAdmin()) {
			Close();  // Exit current non-admin instance
		}
	}
	else {
		wxLogMessage("OnRestartAsAdmin Log: Restarted the app as administrator.");
	}
}

void ApplyThemeToWindow(wxWindow* win, const wxColour& bgColor, const wxColour& fgColor)
{
	if (!win) return;

	win->SetBackgroundColour(bgColor);
	win->SetForegroundColour(fgColor);

	if (auto listCtrl = dynamic_cast<wxListCtrl*>(win))
	{
		listCtrl->SetTextColour(fgColor);
		listCtrl->SetBackgroundColour(bgColor);
	}

	if (auto treeCtrl = dynamic_cast<wxTreeCtrl*>(win))
	{
		treeCtrl->SetBackgroundColour(bgColor);
		treeCtrl->SetForegroundColour(fgColor);
	}

	// Apply theme recursively to children
	const auto& children = win->GetChildren();
	for (wxWindow* child : children)
	{
		ApplyThemeToWindow(child, bgColor, fgColor);
	}

	win->Refresh();
}

void MyFrame::ApplyTheme(const wxString& theme)
{
	if (theme == "Dark") {
		SetBackgroundColour(wxColour(45, 45, 48));
		SetForegroundColour(*wxWHITE);
	}
	else {
		SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
		SetForegroundColour(*wxBLACK);
	}

	treeCtrl->SetBackgroundColour(GetBackgroundColour());
	treeCtrl->SetForegroundColour(GetForegroundColour());
	leftoverTreeCtrl->SetBackgroundColour(GetBackgroundColour());
	leftoverTreeCtrl->SetForegroundColour(GetForegroundColour());

	Refresh();
	Update();
}


// Reading the .ini file and applying
void MyFrame::LoadTheme()
{
	wxFileConfig config("CompleteUninstaller");
	config.SetPath("/Settings");
	currentTheme = config.Read("Theme", "Light"); // podrazumevana svetla
	ApplyTheme(currentTheme);
}

void MyFrame::OnThemeSelect(wxCommandEvent& event)
{
	if (event.GetId() == Theme_Dark)
		currentTheme = "Dark";
	else
		currentTheme = "Light";

	wxFileConfig config("CompleteUninstaller");
	config.SetPath("/Settings");
	config.Write("Theme", currentTheme);
	config.Flush();

	ApplyTheme(currentTheme);
}

// Display leftover items
void MyFrame::DisplayLeftovers(const std::vector<std::wstring>& leftovers, const std::vector<std::wstring>& registryKeys, const std::vector<std::wstring>& services) {
	leftoverTreeCtrl->DeleteAllItems();

	// Single root node
	auto root = leftoverTreeCtrl->AddRoot("Analysis Results");

	// Section: Leftover Files
	auto filesRoot = leftoverTreeCtrl->AppendItem(root, "Detected Leftover Files");
	for (const auto& item : leftovers) {
		leftoverTreeCtrl->AppendItem(filesRoot, wxString(item));
	}

	// Section: Registry Keys
	auto registryRoot = leftoverTreeCtrl->AppendItem(root, "Detected Registry Keys");
	for (const auto& item : registryKeys) {
		leftoverTreeCtrl->AppendItem(registryRoot, wxString(item));
	}

	// Section: Services / Processes
	auto servicesRoot = leftoverTreeCtrl->AppendItem(root, "Detected Running Services");
	for (const auto& item : services) {
		leftoverTreeCtrl->AppendItem(servicesRoot, wxString(item));
	}

	leftoverTreeCtrl->ExpandAll();
}




