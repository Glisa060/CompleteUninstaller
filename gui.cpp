#include "gui.h"
#include "util.h"

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(Minimal_Quit, MyFrame::OnQuit)
EVT_MENU(Minimal_About, MyFrame::OnAbout)
EVT_MENU(Minimal_Open, MyFrame::OnOpen)
EVT_TREE_SEL_CHANGED(Run_Selected, MyFrame::OnTreeSelectionChanged)
wxEND_EVENT_TABLE()


FILE* logFile = nullptr;
std::map<wxString, wxString> installedPrograms;
std::map<wxString, wxString> uninstallerPaths;
std::string selectedUninstallerPath;

wxIMPLEMENT_APP(MyApp);

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(true);
}

void MyFrame::OnProgramListUpdated() {
	GetInstalledPrograms(installedPrograms);
	PopulateTreeView();
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
		wxMessageBox("Uninstaller launched successfully.",
			"Success",
			wxOK | wxICON_INFORMATION,
			this);

		std::string regPath = std::string(command.mb_str());
		wxFileName fileName(command);
		wxString folderName = fileName.GetDirs().Last();
		char* folderNameCopy = _strdup(folderName);
		wxLogInfo("Name that is passed: %s", folderName);
		std::vector<std::wstring> leftovers = CleanUpLeftovers(folderName.ToStdString(), regPath.c_str());
		DisplayLeftovers(leftovers);
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

int MyApp::OnExit()
{
	// Zatvori log fajl ako je otvoren
	if (logFile) {
		fclose(logFile);
		logFile = nullptr;
	}

	return wxApp::OnExit();
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
		std::string key = std::string(uninstallPath.mb_str(wxConvUTF8));

        wxTreeItemId* targetNode = &sys64Node; // Default  

        if (key.find("WOW6432Node") != std::string::npos) {  
            targetNode = &sys32Node;  
        } else if (key.find("HKEY_CURRENT_USER") != std::string::npos || key.find("HKCU") != std::string::npos) {  
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