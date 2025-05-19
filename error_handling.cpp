#include <wx/msgdlg.h>
#include "error_handling.h"


ErrorDialog::ErrorDialog(wxWindow* parent, const wxString& message, const wxString& title)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(300, 200)),
	m_message(message), m_title(title) {
	// You can add other initialization code here
}

void ErrorDialog::ShowErrorMessage() const {
	wxMessageBox(m_message, m_title, wxICON_ERROR | wxOK);
}