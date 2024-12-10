#pragma once
#include "gui.h"


#ifndef wxHAS_IMAGES_IN_RESOURCES
#include "../sample.xpm"
#endif



wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_MENU(Minimal_Quit, MyFrame::OnQuit)
EVT_MENU(Minimal_About, MyFrame::OnAbout)
EVT_MENU(Minimal_Open,MyFrame::OnOpen) // I added this for uninstall
EVT_TREE_SEL_CHANGED(wxID_ANY, MyFrame::OnTreeSelectionChanged)
//EVT_BUTTON(wxID_ANY, MyFrame::OnUninstallButtonClick)
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
    // create a menu bar
    wxMenu* fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");
    fileMenu->Append(Minimal_Open, "&Open\tAlt-O", "Open an item from the list");
    fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");
 

    // now append the freshly created menu to the menu bar...
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);
#else // !wxUSE_MENUBAR
    // If menus are not available add a button to access the about box
    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* aboutBtn = new wxButton(this, wxID_ANY, "About...");
    aboutBtn->Bind(wxEVT_BUTTON, &MyFrame::OnAbout, this);
    sizer->Add(aboutBtn, wxSizerFlags().Center());
    SetSizer(sizer);
#endif // wxUSE_MENUBAR/!wxUSE_MENUBAR

#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText("Complete Uninstaller 0.1 alpha!");
#endif // wxUSE_STATUSBAR
}







