#pragma once
#include <wx/treectrl.h>
#include <wx/event.h>
#include <wx/wx.h>
#include <wx/filename.h>
#include <cstring>
#include <wx/splitter.h> // Include the wxSplitterWindow header
// Ensure wxSplitterWindow is properly included and used  


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
	void OnQuit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	wxDECLARE_EVENT_TABLE();

	wxTreeCtrl* treeCtrl;
	wxTreeCtrl* leftoverTreeCtrl; // Leftover files tree view
	void OnTreeSelectionChanged(wxTreeEvent& event);
	void PopulateTreeView();
	void OnProgramListUpdated();
	void DisplayLeftovers(const std::vector<std::wstring>& leftovers);
};

enum
{
	Minimal_Quit = wxID_EXIT,
	Minimal_About = wxID_ABOUT,
	Minimal_Open = wxID_OPEN,
	Run_Selected = wxID_ANY
};


