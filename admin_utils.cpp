

#include <wx/app.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/frame.h> // Add this include to resolve the incomplete type error for wxFrame
#include <windows.h>

#define WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#include <sddl.h>
#include <ShlObj.h>
#include "admin_utils.h"


bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup))
    {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

bool RestartAsAdmin() {
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(NULL, path, MAX_PATH))
        return false;

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = path;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei))
        return false;

    return true;
}

void runAsAdmin(wxFrame* mainFrame) {
    if (IsUserAnAdmin()) {
        wxLogInfo("runAsAdmin Log: Already running as administrator.");
        return;
    }

    wchar_t currentPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, currentPath, MAX_PATH) == 0) {
        wxLogError("Failed to get current executable path!");
        return;
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = currentPath;
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOASYNC;

    if (!ShellExecuteExW(&sei)) {
        DWORD err = GetLastError();
        wxLogError("Failed to relaunch with admin rights! Error code: %lu", err);
        return;
    }

    wxLogInfo("Program relaunched with admin rights. Exiting current instance...");

    
    if (mainFrame) {
        mainFrame->Close(); // close the window only
    }
    else {
        wxTheApp->ExitMainLoop(); // closes the app safely
    }
}
