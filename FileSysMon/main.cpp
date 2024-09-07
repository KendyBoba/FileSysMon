#include <iostream>
#include "CheckDiff.h"
#include <boost/json/src.hpp>

void WINAPI serviceCtrlHandlerProc(DWORD ctrl,DWORD event_type,void* event_data,void* context) {
	switch (ctrl)
	{
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP: global::instance().is_work = false;break;
	case SERVICE_CONTROL_PAUSE: global::instance().is_pause = true; break;
	case SERVICE_CONTROL_CONTINUE: global::instance().is_pause = false;break;
	case SERVICE_CONTROL_INTERROGATE:; break;
	}
	++global::instance().hServiceStatus.dwCheckPoint;
}

int startService() {
	const boost::filesystem::path prog_data = L"C:\\ProgramData";
	const boost::filesystem::path prog_data_fsm = prog_data / L"FileSysMon";
	boost::filesystem::create_directory(prog_data_fsm);
	SetCurrentDirectory(prog_data_fsm.wstring().c_str());
	boost::filesystem::create_directory(boost::filesystem::current_path() / L"logs");
	boost::filesystem::path error_path = boost::filesystem::current_path() / L"logs\\error.log";
	boost::filesystem::path change_path = boost::filesystem::current_path() / L"logs\\change.log";
	std::ofstream open_object(error_path.wstring(),std::ios::app);
	open_object.close();
	open_object.open(change_path.wstring());
	open_object.close();
	auto error_log = std::make_shared<std::fstream>(error_path.wstring(), std::fstream::in | std::fstream::out | std::fstream::app);
	error_log->flush();
	auto change_log = std::make_shared<std::fstream>(change_path.wstring(), std::fstream::in | std::fstream::out | std::fstream::app);
	bool use_log = error_log->is_open() && change_log->is_open();
	try {
		CheckDiff diff;
		if (use_log)
			diff.setLog(error_log, change_log);
		Storage::instance().signalInsert = std::bind(&CheckDiff::slotAdd, &diff, std::placeholders::_1);
		Storage::instance().signalRemove = std::bind(&CheckDiff::slotRemove, &diff, std::placeholders::_1);
		diff.exec();
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

void WINAPI serviceMain(DWORD argc,LPWSTR argv[]) {
	global::instance().hServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	global::instance().hServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	global::instance().hServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	global::instance().hServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	global::instance().hServiceStatus.dwServiceSpecificExitCode = 0;
	global::instance().hServiceStatus.dwCheckPoint = 0;
	global::instance().hServiceStatus.dwWaitHint = global::instance().ACCWAIT;
	global::instance().hStat = RegisterServiceCtrlHandlerEx(global::instance().service_name, (LPHANDLER_FUNCTION_EX)serviceCtrlHandlerProc,nullptr);
	SetServiceStatus(global::instance().hStat, &global::instance().hServiceStatus);
	if (!startService()) {
		global::instance().UpdateStatus(SERVICE_STOPPED);
		global::instance().hServiceStatus.dwServiceSpecificExitCode = -1;
		SetServiceStatus(global::instance().hStat, &global::instance().hServiceStatus);
		return;
	}
	global::instance().UpdateStatus(SERVICE_STOPPED);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#ifdef DEBUG
	startService();
#else
	SERVICE_TABLE_ENTRY dispatcher_table[]{
		{global::instance().service_name, serviceMain },
		{ NULL,NULL }
	};
	StartServiceCtrlDispatcher(dispatcher_table);
#endif
	return 0;
}