//
// Created by Milan on 30.6.2024
//

#include "../header/gui.h"
#include "../header/util.h"

HWND hWndToolBar = nullptr;
HWND hWndTreeView = nullptr;
std::map<std::string, std::string> uninstallerPaths;
std::string selectedUninstallerPath;
std::map<std::string, std::string> programs;

// Initialization of tree view items
void GetInstalledPrograms(std::map<std::string, std::string>& _programs) {
  HKEY hKey;
  std::vector<std::string> registryPaths = {
      R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall)",
      R"(SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall)"};

  for (const auto& registryPath : registryPaths) {
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, registryPath.c_str(), 0, KEY_READ,
                     &hKey) == ERROR_SUCCESS) {
      TCHAR achKey[MAX_KEY_LENGTH];         // buffer for subkey name
      DWORD cbName;                         // size of name string
      TCHAR achClass[MAX_PATH] = TEXT("");  // buffer for class name
      DWORD cchClassName = MAX_PATH;        // size of class string
      DWORD cSubKeys = 0;                   // number of subkeys
      DWORD cbMaxSubKey;                    // longest subkey size
      DWORD cchMaxClass;                    // longest class string
      DWORD cValues;                        // number of values for key
      DWORD cchMaxValue;                    // longest value name
      DWORD cbMaxValueData;                 // longest value data
      DWORD cbSecurityDescriptor;           // size of security descriptor
      FILETIME ftLastWriteTime;             // last write time

      // Get the class name and the value count.
      RegQueryInfoKey(hKey,                   // key handle
                      achClass,               // buffer for class name
                      &cchClassName,          // size of class string
                      nullptr,                // reserved
                      &cSubKeys,              // number of subkeys
                      &cbMaxSubKey,           // longest subkey size
                      &cchMaxClass,           // longest class string
                      &cValues,               // number of values for this key
                      &cchMaxValue,           // longest value name
                      &cbMaxValueData,        // longest value data
                      &cbSecurityDescriptor,  // security descriptor
                      &ftLastWriteTime);      // last write time

      if (cSubKeys) {
        for (DWORD i = 0; i < cSubKeys; i++) {
          cbName = MAX_KEY_LENGTH;
          if (RegEnumKeyEx(hKey, i, achKey, &cbName, nullptr, nullptr, nullptr,
                           &ftLastWriteTime) == ERROR_SUCCESS) {
            HKEY hSubKey;
            std::string subKeyPath = registryPath + "\\" + achKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath.c_str(), 0,
                             KEY_READ, &hSubKey) == ERROR_SUCCESS) {
              TCHAR displayName[256];
              TCHAR uninstallStr[256];
              DWORD displayNameSize = sizeof(displayName);
              DWORD uninstallStrSize = sizeof(uninstallStr);

              if (RegQueryValueEx(hSubKey, "DisplayName", nullptr, nullptr,
                                  (LPBYTE)displayName,
                                  &displayNameSize) == ERROR_SUCCESS &&
                  RegQueryValueEx(hSubKey, "UninstallString", nullptr, nullptr,
                                  (LPBYTE)uninstallStr,
                                  &uninstallStrSize) == ERROR_SUCCESS) {
                _programs[displayName] = uninstallStr;
              }

              RegCloseKey(hSubKey);
            }
          }
        }
      }

      RegCloseKey(hKey);
    }
  }
}

void GetUserInstalledPrograms(std::map<std::string, std::string>& _programs) {
  HKEY hKey;
  std::string registryPath =
      R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall)";

  if (RegOpenKeyEx(HKEY_CURRENT_USER, registryPath.c_str(), 0, KEY_READ,
                   &hKey) == ERROR_SUCCESS) {
    TCHAR achKey[MAX_KEY_LENGTH];         // buffer for subkey name
    DWORD cbName;                         // size of name string
    TCHAR achClass[MAX_PATH] = TEXT("");  // buffer for class name
    DWORD cchClassName = MAX_PATH;        // size of class string
    DWORD cSubKeys = 0;                   // number of subkeys
    DWORD cbMaxSubKey;                    // longest subkey size
    DWORD cchMaxClass;                    // longest class string
    DWORD cValues;                        // number of values for key
    DWORD cchMaxValue;                    // longest value name
    DWORD cbMaxValueData;                 // longest value data
    DWORD cbSecurityDescriptor;           // size of security descriptor
    FILETIME ftLastWriteTime;             // last write time

    // Get the class name and the value count.
    RegQueryInfoKey(hKey,                   // key handle
                    achClass,               // buffer for class name
                    &cchClassName,          // size of class string
                    nullptr,                // reserved
                    &cSubKeys,              // number of subkeys
                    &cbMaxSubKey,           // longest subkey size
                    &cchMaxClass,           // longest class string
                    &cValues,               // number of values for this key
                    &cchMaxValue,           // longest value name
                    &cbMaxValueData,        // longest value data
                    &cbSecurityDescriptor,  // security descriptor
                    &ftLastWriteTime);      // last write time

    if (cSubKeys) {
      for (DWORD i = 0; i < cSubKeys; i++) {
        cbName = MAX_KEY_LENGTH;
        if (RegEnumKeyEx(hKey, i, achKey, &cbName, nullptr, nullptr, nullptr,
                         &ftLastWriteTime) == ERROR_SUCCESS) {
          HKEY hSubKey;
          std::string subKeyPath = registryPath + "\\" + achKey;
          if (RegOpenKeyEx(HKEY_CURRENT_USER, subKeyPath.c_str(), 0, KEY_READ,
                           &hSubKey) == ERROR_SUCCESS) {
            TCHAR displayName[256];
            TCHAR uninstallStr[256];
            DWORD displayNameSize = sizeof(displayName);
            DWORD uninstallStrSize = sizeof(uninstallStr);

            if (RegQueryValueEx(hSubKey, "DisplayName", nullptr, nullptr,
                                (LPBYTE)displayName,
                                &displayNameSize) == ERROR_SUCCESS &&
                RegQueryValueEx(hSubKey, "UninstallString", nullptr, nullptr,
                                (LPBYTE)uninstallStr,
                                &uninstallStrSize) == ERROR_SUCCESS) {
              _programs[displayName] = uninstallStr;
            }

            RegCloseKey(hSubKey);
          }
        }
      }
    }

    RegCloseKey(hKey);
  }
}

void PopulateTreeView(HWND hwndTV,
                      const std::map<std::string, std::string>& _programs) {
  TVINSERTSTRUCT tvis = {nullptr};

  tvis.hParent = nullptr;
  tvis.hInsertAfter = TVI_ROOT;
  tvis.item.mask = TVIF_TEXT;

  // Add root item
  tvis.item.pszText = const_cast<LPSTR>("Installed Programs");
  auto hRoot = TreeView_InsertItem(hwndTV, &tvis);

  // Add program items
  tvis.hParent = hRoot;

  for (const auto& program : _programs) {
    tvis.item.pszText = const_cast<LPSTR>(program.first.c_str());
    auto hItem = TreeView_InsertItem(hwndTV, &tvis);
    // Optionally handle the uninstaller path stored in program.second
    uninstallerPaths[program.first] = program.second;
    TreeView_Expand(hwndTV, hItem, TVE_EXPAND);
  }

  // Expand the root item
  TreeView_Expand(hwndTV, hRoot, TVE_EXPAND);
}

void InitTreeViewItems(HWND hwndTV) {
  // Clear the programs map to ensure it's empty before populating
  programs.clear();

  // Get installed programs from both HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE
  GetInstalledPrograms(programs);
  GetUserInstalledPrograms(programs);

  // Populate the TreeView with the combined program list
  PopulateTreeView(hwndTV, programs);
}

// Process messages for the main window
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
    case WM_CREATE: {
      createToolbar(hWnd);
      createTreeView(hWnd);
      break;
    }

    case WM_SIZE: {
      SendMessage(hWndToolBar, TB_AUTOSIZE, 0, 0);
      break;
    }
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case TB_TEST1:
          OnButton1Click();
          break;

        case TB_TEST2:
          MessageBox(nullptr, "Toolbar Button Two", "Success",
                     MB_OK | MB_ICONINFORMATION);
          break;
      }
      return 0;

    case WM_NOTIFY:
      if (((LPNMHDR)lParam)->hwndFrom == hWndTreeView &&
          ((LPNMHDR)lParam)->code == TVN_SELCHANGED) {
        OnTreeViewSelectionChanged(hWndTreeView);
      }
      break;

    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return 0;
}

// Function to create the toolbar
void createToolbar(HWND hWnd) {
  TBADDBITMAP tbab;
  TBBUTTON tbb[3];
  TBBUTTONINFO tbbi;

  // Create the toolbar window with TBSTYLE_LIST to support text labels
  hWndToolBar = CreateWindowEx(
      0, TOOLBARCLASSNAME, (LPSTR) nullptr,
      WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_LIST, 0, 0, 0, 0, hWnd,
      (HMENU)IDTB_TOOLBAR, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
      nullptr);

  if (!hWndToolBar) {
    MessageBox(nullptr, "Tool Bar Failed.", "Error", MB_OK | MB_ICONERROR);
    return;
  }

  SendMessage(hWndToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
  SendMessage(hWndToolBar, TB_SETBITMAPSIZE, (WPARAM)0,
              (LPARAM)MAKELONG(20, 20));

  tbab.hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
  tbab.nID = IDB_TOOLBITMAP;
  SendMessage(hWndToolBar, TB_ADDBITMAP, (WPARAM)2, (LPARAM)&tbab);

  ZeroMemory(tbb, sizeof(tbb));
  tbb[0].iBitmap = 0;  // Index of the bitmap in the toolbar's image list
  tbb[0].idCommand = TB_TEST1;
  tbb[0].fsState = TBSTATE_ENABLED;
  tbb[0].fsStyle =
      TBSTYLE_BUTTON |
      BTNS_AUTOSIZE;  // Ensure the button can resize to fit the text

  tbb[1].fsStyle = TBSTYLE_SEP;

  tbb[2].iBitmap = 1;  // Index of the bitmap in the toolbar's image list
  tbb[2].idCommand = TB_TEST2;
  tbb[2].fsState = TBSTATE_ENABLED;
  tbb[2].fsStyle =
      TBSTYLE_BUTTON |
      BTNS_AUTOSIZE;  // Ensure the button can resize to fit the text

  SendMessage(hWndToolBar, TB_ADDBUTTONS, 3, (LPARAM)&tbb);

  // Define button text
  char buttonText1[] = "Uninstall";
  char buttonText2[] = "Button 2";

  // Set button text for TB_TEST1
  ZeroMemory(&tbbi, sizeof(tbbi));
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_TEXT | TBIF_SIZE;
  tbbi.pszText = buttonText1;
  tbbi.cx = 80;  // Adjust the width as necessary
  SendMessage(hWndToolBar, TB_SETBUTTONINFO, TB_TEST1, (LPARAM)&tbbi);

  // Set button text for TB_TEST2
  ZeroMemory(&tbbi, sizeof(tbbi));
  tbbi.cbSize = sizeof(TBBUTTONINFO);
  tbbi.dwMask = TBIF_TEXT | TBIF_SIZE;
  tbbi.pszText = buttonText2;
  tbbi.cx = 80;  // Adjust the width as necessary
  SendMessage(hWndToolBar, TB_SETBUTTONINFO, TB_TEST2, (LPARAM)&tbbi);

  // Adjust the size of the buttons to fit the text
  SendMessage(hWndToolBar, TB_AUTOSIZE, 0, 0);

  // Center the text by adding padding
  SendMessage(hWndToolBar, TB_SETPADDING, 0,
              MAKELPARAM(10, 10));  // Adjust padding as necessary

  ShowWindow(hWndToolBar, SW_SHOW);
}

// Function to create the treeview
void createTreeView(HWND hwnd) {
  RECT rcClient;
  GetClientRect(hwnd, &rcClient);
  hWndTreeView = CreateWindowEx(
      0, WC_TREEVIEW, nullptr, WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES,
      0, 40, rcClient.right / 4, rcClient.bottom - 30, hwnd, nullptr,
      GetModuleHandle(nullptr), nullptr);
  InitTreeViewItems(hWndTreeView);
}

void OnTreeViewSelectionChanged(HWND hwndTV) {
  auto hSelectedItem = TreeView_GetSelection(hwndTV);
  if (hSelectedItem) {
    TVITEM tvi;
    tvi.mask = TVIF_TEXT;
    tvi.hItem = hSelectedItem;
    tvi.pszText = new char[256];
    tvi.cchTextMax = 256;

    if (TreeView_GetItem(hwndTV, &tvi)) {
      std::string selectedProgram = tvi.pszText;
      selectedUninstallerPath = uninstallerPaths[selectedProgram];
    }

    delete[] tvi.pszText;
  }
}

void OnButton1Click() {
  if (!selectedUninstallerPath.empty()) {
    ShellExecute(nullptr, "open", selectedUninstallerPath.c_str(), nullptr,
                 nullptr, SW_SHOWNORMAL);
  } else {
    MessageBox(nullptr,
               "No program selected or the selected program does not have an "
               "uninstaller.",
               "Error", MB_OK | MB_ICONERROR);
  }
}
