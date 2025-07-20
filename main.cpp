

#include <wx/treectrl.h>
#include <wx/splitter.h>
#include <wx/artprov.h> // Include this header to resolve wxArtProvider and related identifiers
#include "gui.h"
#include "util.h"
#include "admin_utils.h"
#include "enums.h"
MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400))
{
    if (IsRunningAsAdmin())
        SetTitle("Complete Uninstaller (Admin)");
    else
        SetTitle("Complete Uninstaller");

 /*   wxString version = wxVERSION_STRING;
    wxLogMessage("Using wxWidgets version: %s", version);*/

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);
    treeCtrl = new wxTreeCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);
    leftoverTreeCtrl = new wxTreeCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);
    splitter->SplitVertically(treeCtrl, leftoverTreeCtrl);

    wxSize size = GetClientSize();
    splitter->SetSashPosition(size.GetWidth() / 2);

    sizer->Add(splitter, 1, wxEXPAND | wxALL, 5);

    //wxIcon testIcon("Icons/delete.png", wxBITMAP_TYPE_PNG);
    //if (!testIcon.IsOk()) {
    //    wxMessageBox("Icon not loaded!");
    //}

    //// Image list & icons
    imageList = new wxImageList(16, 16, true);
    //imageList->Add(wxIcon("Icons/tree_root.png", wxBITMAP_TYPE_PNG));   // ID 0
    //imageList->Add(wxIcon("Icons/user_icon.png", wxBITMAP_TYPE_PNG));   // ID 1
    //imageList->Add(wxIcon("Icons/sys32_icon.png", wxBITMAP_TYPE_PNG));  // ID 2
    //imageList->Add(wxIcon("Icons/sys64_icon.png", wxBITMAP_TYPE_PNG));  // ID 3
    //imageList->Add(wxIcon("Icons/delete.png", wxBITMAP_TYPE_PNG));      // ID 4

	// TODO: Add icons for the tree control

    wxIcon builtin = wxArtProvider::GetIcon(wxART_LIST_VIEW, wxART_OTHER, wxSize(16, 16));
    wxIcon folder = wxArtProvider::GetIcon(wxART_FOLDER, wxART_OTHER, wxSize(16, 16));
    wxIcon exeFile = wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE, wxART_OTHER, wxSize(16, 16));
    imageList->Add(builtin); // test ID 0
    imageList->Add(folder); // test ID 1
    imageList->Add(exeFile); // test ID 1
    

    treeCtrl->AssignImageList(imageList);

    SetSizer(sizer);
    Layout();

    runAsAdmin(this);
    OnProgramListUpdated();

    // Menu
    wxMenu* fileMenu = new wxMenu;
    wxMenu* helpMenu = new wxMenu;
    wxMenu* themeMenu = new wxMenu;

    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");
    helpMenu->Append(Minimal_OpenLog, "&Open Log File\tCtrl+L", "Open the application log file");
    fileMenu->Append(Minimal_Open, "&Run selected\tAlt-O", "Run the uninstaller for the selected item");
    fileMenu->Append(Minimal_Analyse, "&Analyse selected\tAlt-A", "Analyse leftovers of the selected program");
    fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");
    fileMenu->Append(RestartAsAdmin_ID, "Restart as Admin\tCtrl+R", "Restart the app with admin privileges");
    themeMenu->AppendRadioItem(Theme_Light, "Light Theme");
    themeMenu->AppendRadioItem(Theme_Dark, "Dark Theme");

    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(themeMenu, "&Theme");
    menuBar->Append(helpMenu, "&Help");


    SetMenuBar(menuBar);
    CreateStatusBar(2);
    SetStatusText(wxString::Format("Complete Uninstaller 0.1 alpha - %s", wxVERSION_STRING));
}




