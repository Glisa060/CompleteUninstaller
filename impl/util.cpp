//
// Created by Milan on 8.7.2024
//

#include "../header/util.h"

void printErrorMessage(DWORD errorCode) {
  LPVOID lpMsgBuf;
  DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS;

  // Retrieve the system error message for the provided error code
  FormatMessage(dwFlags, NULL, errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0,
                NULL);

  // Print the error message
  std::wcout << L"Error code: " << errorCode << L" - " << (LPCTSTR)lpMsgBuf
             << std::endl;

  // Free the buffer allocated by FormatMessage
  LocalFree(lpMsgBuf);
}
