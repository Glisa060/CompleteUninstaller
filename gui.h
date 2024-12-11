#pragma once
#include <wx/treectrl.h>
#include <wx/wx.h>


class MyApp : public wxApp
{
public:

    virtual bool OnInit() wxOVERRIDE;
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title);

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event); 

private:
    wxDECLARE_EVENT_TABLE();


    wxTreeCtrl* treeCtrl;
    void OnTreeSelectionChanged(wxTreeEvent& event);
    void PopulateTreeView();
    void OnProgramListUpdated();
  


};

enum
{
    Minimal_Quit = wxID_EXIT,
    Minimal_About = wxID_ABOUT,
    Minimal_Open = wxID_OPEN,
    Run_Selected = wxID_ANY
};


