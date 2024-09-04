#include "global.h"


global& global::instance()
{
	static global* inst = new global;
	return *inst;
}

void global::UpdateStatus(DWORD service_state)
{
	++hServiceStatus.dwCheckPoint;
	hServiceStatus.dwCurrentState = service_state;
	SetServiceStatus(hStat, &hServiceStatus);
}
