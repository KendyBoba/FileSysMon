#include <iostream>
#include <stdio.h>
#include <memory.h>
#include <boost/locale.hpp>
#include "ConsoleProgram.h"
#include <Windows.h>
static LPWSTR service_name = (wchar_t*)L"FileSysMon";
static std::wstring event_ui_name = L"Global\\FileSysMonUi";

int wmain(int argc, wchar_t* argv[])
{
	std::setlocale(LC_ALL, "");
	HANDLE event_ui = CreateEvent(NULL, false, false, event_ui_name.c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		std::wcout << "The application is already running\n";
		return -1;
	}
	if (event_ui == NULL) {
		std::wcout << "Application launch check error\n";
		return -1;
	}
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
		binary_path.wstring().c_str(), NULL, NULL, NULL, NULL, NULL); //L"NT AUTHORITY\\LocalService"
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
			std::wcout << L"Error: when checking service status\n";
			std::cin >> exit;
			return -1;
		}
	}
	CloseServiceHandle(hSc);
	try {
		ConsoleProgram prog;
		if (argc <= 1)
			prog.exec();
		else {
			std::wstring w_arg = L"";
			for (short i = 1; i < argc; ++i)
				w_arg.append(argv[i]);
			prog.parse(w_arg);
		}
	}
	catch (boost::interprocess::interprocess_exception &ex) {
		std::cout << ex.what() << std::endl;
		CloseHandle(event_ui);
	}
	catch (std::exception& exep) {
		std::cout << exep.what() << std::endl;
		CloseHandle(event_ui);
	}
	catch (...) {
		std::cout << "Uniknown error" << std::endl;
		CloseHandle(event_ui);
	}
	CloseHandle(event_ui);
	return 0;
}
