#pragma once
#include <wx/string.h>
#include <wx/frame.h>
#include <string>
#include <map>
#include <functional>
#include <wx/filename.h>
#include <wx/stdpaths.h>

void CleanUpLeftovers(const std::string& programName, const wxString& regPath, std::function<void()> onFinished);
wxString GetLogFilePath();
