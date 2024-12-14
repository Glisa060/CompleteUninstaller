#include "gui.h"
#include "util.h"

std::map<wxString, wxString> installedPrograms;
std::map<wxString, std::string> uninstallerPaths;
std::string selectedUninstallerPath;

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

		CleanUpLeftovers();
	}
}


void MyFrame::PopulateTreeView() {
	if (installedPrograms.empty()) {
		wxLogError("No programs found to display in the tree.");
		return;
	}

	treeCtrl->DeleteAllItems();

	wxTreeItemId root = treeCtrl->AddRoot("Installed Programs");

	for (const auto& program : installedPrograms) {
		if (program.first.IsEmpty() || program.second.empty()) {
			wxLogError("Program name or uninstall string is empty.");
			continue;
		}

		wxTreeItemId child = treeCtrl->AppendItem(root, program.first);
		uninstallerPaths[program.first.ToStdString()] = program.second; // Map uninstall string to program name
	}

	treeCtrl->Expand(root);
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
		}
		else {
			wxLogError("Uninstaller path not found for program: %s", programName);
			selectedUninstallerPath.clear();
		}
	}
}