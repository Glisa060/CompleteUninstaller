#include <wx/busyinfo.h>
#include <wx/log.h>
#include <wx/string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <TlHelp32.h>
#include "process_utils.h"

// Detect services and processes
std::vector<std::wstring> SearchServicesAndProcesses(const std::wstring& programName) {
	wxBusyInfo info("Searching services and processes...");
	std::vector<std::wstring> found;

	// Lokking for processes
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32W pe;
		pe.dwSize = sizeof(pe);

		if (Process32FirstW(hSnapshot, &pe)) {
			do {
				if (std::wstring(pe.szExeFile).find(programName) != std::wstring::npos) {
					found.push_back(pe.szExeFile);
					wxLogInfo("Found related process: %s", wxString(pe.szExeFile));
				}
			} while (Process32NextW(hSnapshot, &pe));
		}

		CloseHandle(hSnapshot);
	}

	// Looking for services
	SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (scmHandle) {
		DWORD bytesNeeded, servicesReturned;
		EnumServicesStatusExW(
			scmHandle,
			SC_ENUM_PROCESS_INFO,
			SERVICE_WIN32,
			SERVICE_STATE_ALL,
			nullptr,
			0,
			&bytesNeeded,
			&servicesReturned,
			nullptr,
			nullptr
		);

		std::vector<BYTE> buffer(bytesNeeded);
		LPENUM_SERVICE_STATUS_PROCESSW services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESSW>(buffer.data());

		if (EnumServicesStatusExW(scmHandle, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE,
			buffer.data(), static_cast<DWORD>(buffer.size()), &bytesNeeded, &servicesReturned, nullptr, nullptr)) {

			for (DWORD i = 0; i < servicesReturned; ++i) {
				std::wstring serviceName = services[i].lpServiceName;
				if (serviceName.find(programName) != std::wstring::npos) {
					found.push_back(serviceName);
					wxLogInfo("Found related service: %s", wxString(serviceName));
				}
			}
		}

		CloseServiceHandle(scmHandle);
	}

	return found;    
}

// Stopping and deleting services
bool StopAndDeleteService(const std::wstring& serviceName)
{
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager) return false;

	SC_HANDLE service = OpenServiceW(scManager, serviceName.c_str(), SERVICE_STOP | DELETE);
	if (!service) {
		CloseServiceHandle(scManager);
		return false;
	}

	SERVICE_STATUS status = {};
	ControlService(service, SERVICE_CONTROL_STOP, &status);
	DeleteService(service);

	CloseServiceHandle(service);
	CloseServiceHandle(scManager);
	return true;
}