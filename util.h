#pragma once

#include <wx/treectrl.h>
#include <wx/wx.h>
#include <map>
#include <algorithm>
#include <commctrl.h>
#include <iostream>
#include <vector>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msw/registry.h>
#include <wx/string.h>



class ErrorDialog : public wxDialog {
public:
    ErrorDialog(wxWindow* parent, const wxString& message, const wxString& title);

    void ShowErrorMessage() const;

private:
    wxString m_message;
    wxString m_title;
};;

void GetInstalledPrograms(std::map<wxString, wxString>& programs);
void GetUserInstalledPrograms(std::map<wxString, wxString>& programs);
