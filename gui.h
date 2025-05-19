#pragma once  
#include <wx/wx.h>  
#include <wx/treebase.h>  
#include <map>

class MyApp : public wxApp  
{  
public:  
    virtual bool OnInit() wxOVERRIDE;  
    virtual int OnExit() wxOVERRIDE;  
};  

class MyFrame : public wxFrame  
{  
public:  
    MyFrame(const wxString& title);  

private:  
    wxTreeCtrl* treeCtrl;  
    wxTreeCtrl* leftoverTreeCtrl; // Leftover files tree view  
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
    void OnProgramListReady(std::map<wxString, wxString> programs);  
};  

enum  
{  
    Minimal_Quit = wxID_EXIT,  
    Minimal_About = wxID_ABOUT,  
    Minimal_Open = wxID_OPEN,  
    Run_Selected = wxID_ANY + 1,  
    Minimal_Analyse,  
};
