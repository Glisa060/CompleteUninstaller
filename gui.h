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

    wxListCtrl* leftoversListCtrl;  // To show leftover files/registry keys/services
    wxString currentTheme;
    // Icons
    wxImageList* imageList;
    std::map<wxString, wxString> manufacturers;



    // Data
    wxString uninstallString;
    std::vector<std::wstring> leftovers;

    void DisplayProgramDetails(const std::vector<std::wstring>& leftoverItems);
    void OnRestartAsAdmin(wxCommandEvent&);
    void OnDeleteSelected(wxCommandEvent& event);  

    wxDECLARE_EVENT_TABLE();  

    void OnQuit(wxCommandEvent& event);  
    void OnAbout(wxCommandEvent& event);  
    void OnOpen(wxCommandEvent& event);  

    void OnThemeSelect(wxCommandEvent& event);
    void ApplyTheme(const wxString& theme);
    void LoadTheme();
    void ApplyThemeToWindow(wxWindow* win, const wxColour& bgColor, const wxColour& fgColor);


    void OnTreeSelectionChanged(wxTreeEvent& event);  
    void PopulateTreeView();  
    void OnProgramListUpdated();  
    void DisplayLeftovers(const std::vector<std::wstring>& leftovers, const std::vector<std::wstring>& registryKeys, const std::vector<std::wstring>& services);
    void OnAnalyseMenu(wxCommandEvent& event);
    void OnOpenLogFile(wxCommandEvent& WXUNUSED(event));
};



