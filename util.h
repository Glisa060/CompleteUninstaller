#pragma once
#include <wx/string.h>
#include <wx/frame.h>
#include <string>
#include <map>
#include <functional>


void GetInstalledPrograms(std::map<wxString, wxString>& programs);
void runAsAdmin(wxFrame* mainFrame = nullptr);
void CleanUpLeftovers(const std::string& programName, const wxString& regPath, std::function<void()> onFinished);