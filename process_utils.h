#pragma once
#include <string>
#include <vector>

std::vector<std::wstring> SearchServicesAndProcesses(const std::wstring& programName);
bool StopAndDeleteService(const std::wstring& serviceName);