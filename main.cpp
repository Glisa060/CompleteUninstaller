#pragma once
#include "gui.h"


#ifndef wxHAS_IMAGES_IN_RESOURCES
#include "../sample.xpm"
#endif

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(Minimal_Quit, MyFrame::OnQuit)
EVT_MENU(Minimal_About, MyFrame::OnAbout)
EVT_MENU(Minimal_Open,MyFrame::OnOpen)
EVT_TREE_SEL_CHANGED(Run_Selected, MyFrame::OnTreeSelectionChanged)
wxEND_EVENT_TABLE()


wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    FILE* logFile = fopen("logfile.txt", "w");

    if (logFile == nullptr) {
        wxLogError("Failed to open log file!");
        return false;
    }

    wxLog::SetActiveTarget(new wxLogStderr(logFile, wxConvUTF8));    
    MyFrame* frame = new MyFrame("Complete Uninstaller 0.1 alpha");

    frame->Show(true);

    return true;
}

MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400))
{

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    SetIcon(wxICON(sample));

    treeCtrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);

    sizer->Add(treeCtrl, 1, wxEXPAND | wxALL, 5);

    SetSizer(sizer);

    OnProgramListUpdated();
    PopulateTreeView();

#if wxUSE_MENUBAR
    wxMenu* fileMenu = new wxMenu;

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");
    fileMenu->Append(Minimal_Open, "&Run selected\tAlt-O", "Run the uninstaller for the selected item");
    fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");
 

    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");

    SetMenuBar(menuBar);
#else // !wxUSE_MENUBAR
    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* aboutBtn = new wxButton(this, wxID_ANY, "About...");
    aboutBtn->Bind(wxEVT_BUTTON, &MyFrame::OnAbout, this);
    sizer->Add(aboutBtn, wxSizerFlags().Center());
    SetSizer(sizer);
#endif 

#if wxUSE_STATUSBAR
   
    CreateStatusBar(2);
    SetStatusText("Complete Uninstaller 0.1 alpha!");
#endif 
}







