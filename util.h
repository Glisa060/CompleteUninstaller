#pragma once

#include <wx/treectrl.h>
#include <wx/wx.h>
#include <map>
#include <wx/log.h>
#include <wx/string.h>
#include <winreg.h>
#include <wx/msgdlg.h>




class ErrorDialog : public wxDialog {
public:
	ErrorDialog(wxWindow* parent, const wxString& message, const wxString& title);

	void ShowErrorMessage() const;

private:
	wxString m_message;
	wxString m_title;
};;

void GetInstalledPrograms(std::map<wxString, wxString>& programs);

void runAsAdmin();