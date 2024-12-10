#include "gui.h"
#include "util.h"

std::map<wxString, wxString> installedPrograms;
std::map<wxString, wxString> userInstalledPrograms;
std::map<wxString, std::string> uninstallerPaths;
std::string selectedUninstallerPath;


void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void MyFrame::OnProgramListUpdated() {
    GetUserInstalledPrograms(userInstalledPrograms);  
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
    wxMessageBox(wxString::Format
    (
        "To be implemented",
        wxGetLibraryVersionInfo().GetVersionString(),
        wxGetOsDescription()
    ),
        "To be implemented",
        wxOK | wxICON_INFORMATION,
        this);
}

void MyFrame::PopulateTreeView() {
    if (userInstalledPrograms.empty() && installedPrograms.empty()) {
        wxLogError("No programs found to display in the tree.");
        return;
    }

    std::map<wxString, wxString> programs;

    for (const auto& program : userInstalledPrograms) {
        if (program.first.IsEmpty() || program.second.IsEmpty()) {
            wxLogError("Program name or uninstall path is empty.");
            continue;
        }
        programs[program.first] = program.second;
    }

    for (const auto& program : installedPrograms) {
        if (program.first.IsEmpty() || program.second.IsEmpty()) {
            wxLogError("Program name or uninstall path is empty.");
            continue;
        }
        programs[program.first] = program.second;
    }

    if (programs.empty()) {
        wxLogError("No valid programs found to display.");
        return;
    }

    wxTreeItemId root = treeCtrl->AddRoot("Installed Programs");

    for (const auto& program : programs) {
        if (program.first.IsEmpty()) {
            wxLogError("Program name is empty.");
            continue;
        }

        wxTreeItemId child = treeCtrl->AppendItem(root, program.first);
        uninstallerPaths[program.first] = program.second;
    }

    treeCtrl->Expand(root);
}



void MyFrame::OnTreeSelectionChanged(wxTreeEvent& event) {
    wxTreeItemId selected = event.GetItem();
    if (selected.IsOk()) {
        wxString programName = treeCtrl->GetItemText(selected);
        selectedUninstallerPath = uninstallerPaths[programName.ToStdString()];
    }
}

