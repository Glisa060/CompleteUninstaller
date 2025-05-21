#pragma once
#include <wx/dialog.h>
#include <wx/string.h>

class ErrorDialog : public wxDialog {
public:
	ErrorDialog(wxWindow* parent, const wxString& message, const wxString& title);

	void ShowErrorMessage() const;

private:
	wxString m_message;
	wxString m_title;
};