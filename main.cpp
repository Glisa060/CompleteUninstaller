

#include <wx/treectrl.h>
#include <wx/splitter.h>
#include "gui.h"
#include "util.h"
#include "admin_utils.h"

MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400))
{
    if (IsRunningAsAdmin()) {
        SetTitle("Complete Uninstaller (Admin)");
    }
    else {
        SetTitle("Complete Uninstaller");
    }
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Create the uninstall string list control and add it to the sizer
    uninstallListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 50), wxLC_REPORT);

	// Calculate the width for the column
    wxSize size = GetClientSize();
    int halfWidth = size.GetWidth() / 2;

	// Add columns to the uninstallListCtrl
    uninstallListCtrl->InsertColumn(0, "Uninstall String",0, halfWidth);

    // Add the uninstallListCtrl at the top
    sizer->Add(uninstallListCtrl, 0, wxEXPAND | wxALL, 5);

    // Create the splitter and add tree controls
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);
    treeCtrl = new wxTreeCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);
    leftoverTreeCtrl = new wxTreeCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);
    splitter->SplitVertically(treeCtrl, leftoverTreeCtrl);

    // Set initial sash position
    splitter->SetSashPosition(halfWidth);

    // Add splitter to the main sizer
    sizer->Add(splitter, 1, wxEXPAND | wxALL, 5);

    // Apply sizer
    SetSizer(sizer);
    Layout();

    runAsAdmin(this);
    OnProgramListUpdated();

    // Menu setup
    wxMenu* fileMenu = new wxMenu;
    wxMenu* helpMenu = new wxMenu;

    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");
    fileMenu->Append(Minimal_Open, "&Run selected\tAlt-O", "Run the uninstaller for the selected item");
    fileMenu->Append(Minimal_Analyse, "&Analyse selected\tAlt-A", "Analyse leftovers of the selected program");
    fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");
    fileMenu->Append(RestartAsAdmin_ID, "Restart as Admin\tCtrl+R", "Restart the app with admin privileges");
 
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);
    CreateStatusBar(2);
    SetStatusText("Complete Uninstaller 0.1 alpha!");
}








