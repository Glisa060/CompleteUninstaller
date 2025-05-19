
#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/splitter.h>
#include <wx/busyinfo.h>
#include "gui.h"
#include "util.h"



MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // Prvo napravi splitter
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);

    // Sada kreiraj tree view-ove sa splitterom kao roditeljem
    treeCtrl = new wxTreeCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);

    leftoverTreeCtrl = new wxTreeCtrl(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT);

    splitter->SplitVertically(treeCtrl, leftoverTreeCtrl);
    wxSize size = GetClientSize(); 
    int halfWidth = size.GetWidth() / 2;
    splitter->SetSashPosition(halfWidth);
    sizer->Add(splitter, 1, wxEXPAND | wxALL, 5);

    SetSizer(sizer); 

    runAsAdmin(this);
    OnProgramListUpdated();

    // Menu setup
    wxMenu* fileMenu = new wxMenu;
    wxMenu* helpMenu = new wxMenu;

    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");
    fileMenu->Append(Minimal_Open, "&Run selected\tAlt-O", "Run the uninstaller for the selected item");
    fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");
    fileMenu->Append(Minimal_Analyse, "&Analyse selected\tAlt-A", "Analyse leftovers of the selected program");

    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");

    SetMenuBar(menuBar);
    CreateStatusBar(2);
    SetStatusText("Complete Uninstaller 0.1 alpha!");
}








