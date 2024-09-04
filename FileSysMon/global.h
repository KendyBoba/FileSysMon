#pragma once
#include <Windows.h>
#include <functional>

class global {
public:
	DWORD ACCWAIT = 2000;
	LPWSTR service_name = (wchar_t*)L"FileSysMon";
	SERVICE_STATUS hServiceStatus;
	SERVICE_STATUS_HANDLE hStat;
	bool is_work = true, is_pause = false;
public:
	static global& instance();
	void UpdateStatus(DWORD service_state);
};