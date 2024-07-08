#include "header/gui.h"

// The entry point for a Windows application
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  // Initialize common controls
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_TREEVIEW_CLASSES;
  InitCommonControlsEx(&icex);

  // Register the window class
  const char CLASS_NAME[] = "Sample Window Class";

  WNDCLASS wc = {};
  wc.lpfnWndProc = WindowProc;    // Window procedure callback
  wc.hInstance = hInstance;       // Instance handle
  wc.lpszClassName = CLASS_NAME;  // Window class name
  wc.hIcon =
      LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));  // Application icon
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);         // Cursor style
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);       // Background color
  wc.lpszMenuName = nullptr;                           // Menu resource name

  if (!RegisterClass(&wc)) {
    MessageBox(nullptr, "Failed To Register The Window Class.", "Error",
               MB_OK | MB_ICONERROR);
    return 0;
  }

  // Create the window
  HWND hwnd =
      CreateWindowEx(WS_EX_CLIENTEDGE,  // Optional window styles
                     CLASS_NAME,        // Window class
                     "Complete Uninstaller 0.1 Alpha",  // Window text
                     WS_OVERLAPPEDWINDOW,               // Window style
                     // Size and position
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                     nullptr,    // Parent window
                     nullptr,    // Menu
                     hInstance,  // Instance handle
                     nullptr     // Additional application data
      );

  if (!hwnd) {
    MessageBox(nullptr, "Window Creation Failed.", "Error",
               MB_OK | MB_ICONERROR);
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  // Run the message loop
  MSG msg = {};
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}
