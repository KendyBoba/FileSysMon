#include <iostream>
#include <stdio.h>
#include <memory.h>
#include "ConsoleProgram.h"
#include <Windows.h>
#define _SERVICE
static LPWSTR service_name = (wchar_t*)L"FileSysMon";

int main()
{
	std::setlocale(LC_ALL, "");
#ifdef _SERVICE
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
	SC_HANDLE service = CreateService(hSc,
		service_name,
		L"FileSysMon", SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
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
	try {
		ConsoleProgram prog;
		prog.exec();
	}
	catch (boost::interprocess::interprocess_exception &ex) {
		std::string e = ex.what();
		std::cout << e << std::endl;
	}
	catch (...) {
		std::cout << "Uniknown error" << std::endl;
	}
	return 0;
}
