//
// Created by Milan on 30.6.2024
//
#ifndef WINDOWSGUI_GUI_H
#define WINDOWSGUI_GUI_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

#define IDI_ICON 101
#define IDTB_TOOLBAR 1000
#define IDB_TOOLBITMAP 1001
#define TB_TEST1 1002
#define TB_TEST2 1003
#define MAX_KEY_LENGTH 255

// Toolbar creation function
void createToolbar(HWND hwnd);

// TreeView creation function
void createTreeView(HWND hwnd);

// Function declarations
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
std::vector<std::pair<std::string, std::string>> GetInstalledPrograms(HKEY hKeyRoot);
void PopulateTreeView(HWND hwndTV, const std::vector<std::pair<std::string, std::string>> &programs);
void InitTreeViewItems(HWND hwndTV);
void OnButton1Click();
void OnTreeViewSelectionChanged(HWND hwndTV);
void GetUserInstalledPrograms(std::map<std::string, std::string>& _programs);

#endif // WINDOWSGUI_GUI_H
