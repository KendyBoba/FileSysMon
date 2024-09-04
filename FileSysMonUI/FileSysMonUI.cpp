// FileSysMonUI.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <stdio.h>
#include <memory.h>
#include "ConsoleProgram.h"
#include <Windows.h>

static LPWSTR service_name = (wchar_t*)L"FileSysMon";

int main()
{
	std::setlocale(LC_ALL, "");
#ifndef _DEBUG
	int exit = 0;
	SC_HANDLE hSc = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSc) {
		std::wcout << L"Error: manager was not open\n";
		std::cin >> exit;
		return -1;
	}
	auto binary_path = boost::filesystem::current_path() += L"\\";
	binary_path += service_name;
	binary_path += L".exe";
	std::wcout << binary_path.wstring() << std::endl;
	SC_HANDLE service = CreateService(hSc,
		service_name,
		L"FileSysMon", SERVICE_ALL_ACCESS,
		SERVICE_WIN32_SHARE_PROCESS,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		binary_path.wstring().c_str(), NULL, NULL, NULL, NULL, NULL);
	if (!service && GetLastError() == ERROR_SERVICE_EXISTS) {
		service = OpenService(hSc, service_name, SC_MANAGER_ALL_ACCESS);
	}
	if (!service) {
		std::wcout << L"Error: service was not open\n";
		std::cin >> exit;
		return -1;
	}
	if (!StartService(service, 0, NULL) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
		std::wcout << L"Error: service cannot start\n";
		std::cin >> exit;
		return -1;
	}
	SERVICE_STATUS hss{ 0 };
	while (hss.dwCurrentState != SERVICE_RUNNING) {
		if (!QueryServiceStatus(service, &hss)) {
			std::wcout << L"error when checking service status\n";
			std::cin >> exit;
			return -1;
		}
	}
	CloseServiceHandle(hSc);
#endif
#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::microseconds(1000));
#endif
	try {
		ConsoleProgram prog;
		prog.exec();
	}
	catch (boost::interprocess::interprocess_exception &ex) {
		std::string e = ex.what();
		int i = 0;
	}
	return 0;
}
