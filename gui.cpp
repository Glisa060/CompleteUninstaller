#include <wx/app.h>
#include <wx/busyinfo.h>
#include <wx/filename.h>
#include <wx/treectrl.h>
#include <shlobj.h>
#include "ThreadedSearchHelper.h"
#include "file_utils.h"
#include "process_utils.h"
#include "registry_utils.h"
#include "gui.h"
#include "util.h"

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(Minimal_Quit, MyFrame::OnQuit)
EVT_MENU(Minimal_About, MyFrame::OnAbout)
EVT_MENU(Minimal_Open, MyFrame::OnOpen)
EVT_MENU(Minimal_Analyse, MyFrame::OnAnalyseMenu)
EVT_TREE_SEL_CHANGED(Run_Selected, MyFrame::OnTreeSelectionChanged)
wxEND_EVENT_TABLE()


FILE* logFile = nullptr;
std::map<wxString, wxString> installedPrograms;
std::map<wxString, wxString> uninstallerPaths;
std::map<wxString, wxString> registryPaths;
std::string selectedUninstallerPath;

wxIMPLEMENT_APP(MyApp);

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(true);
}


void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxString::Format
	(
		"Welcome to Complete Uninstaller 0.1 alpha build!\n"
		"\n"
		"Complete uninstaller for Windows!\n"
		"running under %s.",
		wxGetLibraryVersionInfo().GetVersionString(),
		wxGetOsDescription()
	),
		"About Complete Uninstaller",
		wxOK | wxICON_INFORMATION,
		this);
}

void MyFrame::OnOpen(wxCommandEvent& WXUNUSED(event)) {
	if (selectedUninstallerPath.empty()) {
		wxMessageBox("Please select a program from the tree to uninstall.",
			"No Selection",
			wxOK | wxICON_WARNING,
			this);
		return;
	}

	wxString command = wxString(selectedUninstallerPath).Trim(true).Trim(false);

	if (command.StartsWith("MsiExec.exe")) {
		wxLogMessage("Executing MSI uninstaller: %s", command);
	}
	else {
		if (command.StartsWith("\"") && command.EndsWith("\"")) {
			command = command.Mid(1, command.Length() - 2);
		}

		if (!wxFileExists(command)) {
			wxMessageBox("The uninstaller path is invalid or does not exist.",
				"Error",
				wxOK | wxICON_ERROR,
				this);
			wxLogError("Uninstaller path does not exist: %s", command);
			return;
		}

		wxLogMessage("Executing standard uninstaller: %s", command);
	}

	long result = wxExecute(command, wxEXEC_ASYNC);

	if (result == -1) {
		wxMessageBox("Failed to launch the uninstaller.",
			"Error",
			wxOK | wxICON_ERROR,
			this);
		wxLogError("Failed to execute: %s", command);
	}
	else {
        wxFileName fileName(command.ToStdString()); // Explicitly convert wxString to std::string to avoid ambiguity
		wxMessageBox("Uninstaller launched successfully.",
			"Success",
			wxOK | wxICON_INFORMATION,
			this);

		// Pripremi podatke za asinhroni cleanup
		std::string folderNameStr;
		{
			wxFileName fileName(command);
			wxString folderName = fileName.GetDirs().Last();
			folderNameStr = folderName.ToStdString();
			wxLogInfo("Name that is passed: %s", folderName);
		}

		// regPath ti treba da bude validan registar path, ne ceo command string.
		// Ovde pretpostavljam da imaš spremljeno u mapu recimo ili mozes koristiti selected program name
		// Zameni ovo sa stvarnim registry pathom
		std::string regPath = GetRegistryPathForProgram(folderNameStr);

		// Pozovi asinhroni cleanup
		CleanUpLeftovers(folderNameStr, wxString(regPath), [this]() {
			wxMessageBox("Cleanup finished.", "Done", wxICON_INFORMATION, this);
			// Eventualno refresuj UI ili prikazi rezultate
		});
	}
}


bool MyApp::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	logFile = fopen("logfile.txt", "w");

	if (logFile == nullptr) {
		wxLogError("Failed to open log file!");
		return false;
	}

	wxLog::SetActiveTarget(new wxLogStderr(logFile, wxConvUTF8));
	MyFrame* frame = new MyFrame("Complete Uninstaller 0.1 alpha");

	frame->Show(true);

	return true;
}

void MyFrame::PopulateTreeView() {
	if (installedPrograms.empty()) {
		wxLogError("No programs found to display in the tree.");
		return;
	}

	treeCtrl->DeleteAllItems();
	wxTreeItemId root = treeCtrl->AddRoot("Installed Programs");

	wxTreeItemId userNode = treeCtrl->AppendItem(root, "[User] Installed");
	wxTreeItemId sys32Node = treeCtrl->AppendItem(root, "[32-bit] System");
	wxTreeItemId sys64Node = treeCtrl->AppendItem(root, "[64-bit] System");

	for (const auto& [programName, uninstallPath] : installedPrograms) {
		std::string key = uninstallPath.ToStdString();

		wxTreeItemId* targetNode = &sys64Node; // podrazumevano

		if (key.find("WOW6432Node") != std::string::npos) {
			targetNode = &sys32Node;
		}
		else if (key.find("HKEY_CURRENT_USER") != std::string::npos || key.find("HKCU") != std::string::npos) {
			targetNode = &userNode;
		}

		wxTreeItemId item = treeCtrl->AppendItem(*targetNode, programName);
		uninstallerPaths[programName.ToStdString()] = uninstallPath.ToStdString();
	}

	treeCtrl->ExpandAll();
}

void MyFrame::OnTreeSelectionChanged(wxTreeEvent& event) {
	wxTreeItemId selected = event.GetItem();
	if (selected.IsOk()) {
		wxString programName = treeCtrl->GetItemText(selected);
		wxLogMessage("Selected program: %s", programName);

		auto it = uninstallerPaths.find(programName.ToStdString());
		if (it != uninstallerPaths.end()) {
			selectedUninstallerPath = it->second;
			wxLogMessage("Uninstaller path found: %s", selectedUninstallerPath);
			SetStatusText(wxString::Format("Uninstaller: %s", selectedUninstallerPath), 1);
		}
		else {
			wxLogError("Uninstaller path not found for program: %s", programName);
			SetStatusText("Uninstaller: <not found>", 1);
			selectedUninstallerPath.clear();
		}
	}
}

void MyFrame::DisplayLeftovers(const std::vector<std::wstring>& leftovers) {
	leftoverTreeCtrl->DeleteAllItems();
	wxTreeItemId root = leftoverTreeCtrl->AddRoot("Detected Leftovers");

	for (const auto& leftover : leftovers) {
		leftoverTreeCtrl->AppendItem(root, wxString(leftover));
	}

	leftoverTreeCtrl->ExpandAll();
}

void MyFrame::OnAnalyseMenu(wxCommandEvent& event)
{
	wxTreeItemId selected = treeCtrl->GetSelection();

	if (!selected.IsOk()) {
		wxMessageBox("Please select a program from the tree to analyse.", "No Selection", wxICON_INFORMATION);
		return;
	}

	wxString programNameWx = treeCtrl->GetItemText(selected);
	std::string programName = programNameWx.ToStdString();

	auto uninstallIt = uninstallerPaths.find(programName);
	if (uninstallIt == uninstallerPaths.end()) {
		wxMessageBox("Uninstaller path not found for the selected program.", "Error", wxICON_ERROR);
		return;
	}

	auto regIt = registryPaths.find(programName);
	if (regIt == registryPaths.end()) {
		wxMessageBox("Registry path not found for selected program.", "Error", wxICON_ERROR);
		return;
	}

	// Pravimo vektor registry putanja sa jednim elementom iz regIt->second
	std::vector<std::wstring> registryPathsVec = { std::wstring(regIt->second.begin(), regIt->second.end()) };

	// Prikazujemo busy info tokom analize
	wxBusyInfo busy("Analysing leftovers, please wait...", this);

	RunInBackground<std::vector<std::wstring>>(
		[programName, registryPathsVec]() {
		// Ovo se izvršava u worker thread-u
		std::vector<std::wstring> filePaths = {
			GetProgramFilesDir(),
			GetProgramFilesX86Dir(),
			GetKnownFolderPath(FOLDERID_LocalAppData),
			GetKnownFolderPath(FOLDERID_RoamingAppData),
			GetKnownFolderPath(FOLDERID_ProgramData)
		};

		// Traži fajlove leftover-e
		auto leftoverFiles = SearchLeftoverFiles(filePaths, programName);

		// Traži registry leftover-e
		auto leftoverRegistry = SearchRegistryKeys(registryPathsVec, wxString(programName));

		// Traži servise i procese
		auto leftoverServices = SearchServicesAndProcesses(std::wstring(programName.begin(), programName.end()));

		// Spoji sve rezultate u jedan vektor
		std::vector<std::wstring> allLeftovers;
		allLeftovers.reserve(leftoverFiles.size() + leftoverRegistry.size() + leftoverServices.size());
		allLeftovers.insert(allLeftovers.end(), leftoverFiles.begin(), leftoverFiles.end());
		allLeftovers.insert(allLeftovers.end(), leftoverRegistry.begin(), leftoverRegistry.end());
		allLeftovers.insert(allLeftovers.end(), leftoverServices.begin(), leftoverServices.end());

		return allLeftovers;
	},
		[this](std::vector<std::wstring> leftovers) {
		// Ovo se izvršava u glavnom thread-u (GUI)
		if (leftovers.empty()) {
			wxMessageBox("No leftover files, registry entries or services found.", "Clean", wxICON_INFORMATION);
		}
		else {
			DisplayLeftovers(leftovers);
		}
	}
	);
}


void MyFrame::OnProgramListReady(std::map<wxString, wxString> programs)
{
	installedPrograms = std::move(programs);

	wxTreeItemId rootId = treeCtrl->AddRoot("Installed Programs");

	for (const auto& [name, path] : installedPrograms) {
		treeCtrl->AppendItem(rootId, name);
	}

	treeCtrl->Expand(rootId);
}

void MyFrame::OnProgramListUpdated()
{
	wxBusyInfo* info = new wxBusyInfo("Loading installed programs...");

	std::thread([this, info]() {
		std::map<wxString, wxString> programs;
		GetInstalledPrograms(programs);  // Ova funkcija ne sme koristiti GUI direktno!

		// Update GUI u glavnom threadu
		wxTheApp->CallAfter([this, programs = std::move(programs), info]() {
			delete info; // sakrij "busy" indikator
			OnProgramListReady(programs);
		});

	}).detach(); // pozadinski thread
}

std::string MyFrame::GetRegistryPathForProgram(const std::string& programName) {
	auto it = registryPaths.find(programName);
	if (it != registryPaths.end()) {
		return std::string(it->second.mb_str());
	}
	else {
		wxLogWarning("Registry path not found for program: %s", programName);
		return std::string();  // prazan string
	}
}

int MyApp::OnExit() {
	if (logFile) {
		fclose(logFile); // ako koristiš log fajl
		logFile = nullptr;
	}
	return 0;
}

