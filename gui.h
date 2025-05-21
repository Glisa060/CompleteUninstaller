#pragma once  
#include <wx/wx.h>  
#include <wx/treebase.h>  
#include <wx/listctrl.h>
#include <map>


class MyApp : public wxApp  
{  
public:  
    virtual bool OnInit() wxOVERRIDE;  
    virtual int OnExit() wxOVERRIDE;  
private:
    FILE* logFile = nullptr;
};  

class MyFrame : public wxFrame  
{  
public:  
    MyFrame(const wxString& title);  

private:  
    wxTreeCtrl* treeCtrl;  
    wxTreeCtrl* leftoverTreeCtrl; // Leftover files tree view  

    wxListCtrl* uninstallListCtrl = nullptr; // To show uninstall string(s)
    wxListCtrl* leftoversListCtrl;  // To show leftover files/registry keys/services

    // Data
    wxString uninstallString;
    std::vector<std::wstring> leftovers;

    void DisplayUninstallString(const wxString& uninstallStr);
    void DisplayProgramDetails(const std::vector<std::wstring>& leftoverItems);
    void OnRestartAsAdmin(wxCommandEvent&);
    void OnDeleteSelected(wxCommandEvent& event);

    // Button to delete selected leftovers
    wxButton* deleteButton;

    std::string GetRegistryPathForProgram(const std::string& programName);  

    wxDECLARE_EVENT_TABLE();  

    void OnQuit(wxCommandEvent& event);  
    void OnAbout(wxCommandEvent& event);  
    void OnOpen(wxCommandEvent& event);  

    void OnTreeSelectionChanged(wxTreeEvent& event);  
    void PopulateTreeView();  
    void OnProgramListUpdated();  
    void DisplayLeftovers(const std::vector<std::wstring>& leftovers);  
    void OnAnalyseMenu(wxCommandEvent& event);  
};  

enum  
{  
    Minimal_Quit = wxID_EXIT,  
    Minimal_About = wxID_ABOUT,  
    Minimal_Open = wxID_OPEN,  
    Run_Selected = wxID_ANY + 1,  
    Minimal_Analyse, 
    RestartAsAdmin_ID
};
