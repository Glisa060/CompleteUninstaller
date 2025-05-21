#pragma once
#include <string>

// Returns true if the current process is running with administrator privileges.
bool IsRunningAsAdmin();

// Attempts to restart the current process with elevated privileges using UAC.
// Returns true if the elevation process was launched successfully, false otherwise.
bool RestartAsAdmin();

void runAsAdmin(wxFrame* mainFrame = nullptr);
