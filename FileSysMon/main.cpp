#include <iostream>
#include "CheckDiff.h"
#include <boost/json/src.hpp>

static bool is_work = true, is_pause = false;

#ifndef _DEBUG
static DWORD ACCWAIT = 2000;
static LPWSTR service_name = (wchar_t*)L"FileSysMon";
static SERVICE_STATUS hServiceStatus;
static SERVICE_STATUS_HANDLE hStat;

void UpdateStatus(DWORD new_state) {
	++hServiceStatus.dwCheckPoint;
	hServiceStatus.dwCurrentState = new_state;
	SetServiceStatus(hStat, &hServiceStatus);
}

void WINAPI serviceCtrlHandlerProc(DWORD ctrl,DWORD event_type,void* event_data,void* context) {
	switch (ctrl)
	{
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP: is_work = false;break;
	case SERVICE_CONTROL_PAUSE: is_pause = true; break;
	case SERVICE_CONTROL_CONTINUE: is_pause = false;break;
	case SERVICE_CONTROL_INTERROGATE:; break;
	}
	++hServiceStatus.dwCheckPoint;
}
#endif

int startService(const std::wstring& current_dir_path = L"D:") {
	SetCurrentDirectory(current_dir_path.c_str());
	auto call_back_status = [](int status)->void {
#ifndef _DEBUG
		switch (status)
		{
		case CheckDiff::status::STOP: UpdateStatus(SERVICE_STOP_PENDING); break;
		case CheckDiff::status::WORK: UpdateStatus(SERVICE_RUNNING); break;
		}
#endif 
	};
	boost::filesystem::create_directory(boost::filesystem::current_path() / L"logs");
	boost::filesystem::path error_path = boost::filesystem::current_path() / L"logs\\error.log";
	boost::filesystem::path change_path = boost::filesystem::current_path() / L"logs\\change.log";
	std::ofstream open_object(error_path.wstring(),std::ios::app);
	open_object.close();
	open_object.open(change_path.wstring());
	open_object.close();
	auto error_log = std::make_shared<std::fstream>(error_path.wstring(), std::fstream::in | std::fstream::out | std::fstream::app);
	auto change_log = std::make_shared<std::fstream>(change_path.wstring(), std::fstream::in | std::fstream::out | std::fstream::app);
	bool use_log = error_log->is_open() && change_log->is_open();
	try {
		CheckDiff diff(is_work, is_pause);
		if (use_log)
			diff.setLog(error_log, change_log);
		Storage::instance().signalInsert = std::bind(&CheckDiff::slotAdd, &diff, std::placeholders::_1);
		Storage::instance().signalRemove = std::bind(&CheckDiff::slotRemove, &diff, std::placeholders::_1);
		diff.exec(call_back_status);
	}
	catch (boost::filesystem::filesystem_error& exep) {
		std::string str_exept = exep.what();
		*error_log << str_exept << std::endl;
		error_log->close();
		change_log->close();
		return -1;
	}
	catch (boost::interprocess::interprocess_exception& ex) {
		std::string str_exept = ex.what();
		*error_log << str_exept << std::endl;
		error_log->close();
		change_log->close();
		return -1;
	}
	catch (std::exception& ex) {
		*error_log << ex.what() << std::endl;
		error_log->close();
		change_log->close();
		return -1;
	}
	catch (...) {
		*error_log << "any exeption\n" << std::endl;
		error_log->close();
		change_log->close();
		return -1;
	}
	error_log->close();
	change_log->close();
	return 0;
}

#ifndef _DEBUG
void WINAPI serviceMain(DWORD argc,LPWSTR argv[]) {
	hServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	hServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	hServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	hServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	hServiceStatus.dwServiceSpecificExitCode = 0;
	hServiceStatus.dwCheckPoint = 0;
	hServiceStatus.dwWaitHint = ACCWAIT;
	hStat = RegisterServiceCtrlHandlerEx(service_name, (LPHANDLER_FUNCTION_EX)serviceCtrlHandlerProc,nullptr);
	SetServiceStatus(hStat, &hServiceStatus);
	if (!startService(argv[1])) {
		UpdateStatus(SERVICE_STOPPED);
		hServiceStatus.dwServiceSpecificExitCode = -1;
		SetServiceStatus(hStat, &hServiceStatus);
		return;
	}
	UpdateStatus(SERVICE_STOPPED);
}

#endif

int main() {
#ifndef _DEBUG
	SERVICE_TABLE_ENTRY dispatcher_table[]{
		{ service_name, serviceMain },
		{ NULL,NULL }
	};
	StartServiceCtrlDispatcher(dispatcher_table);
	return 0;
#endif
	startService(boost::filesystem::current_path().wstring());
	return 0;
}