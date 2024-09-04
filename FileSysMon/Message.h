#pragma once
#include "FileInfo.h"

enum class MsgType {
	REQUEST,
	RESPONSE,
};

enum class Command {
	NONE,
	_ERROR_,
	_ERROR_BAD_ARG_,
	CONTINUE,
	INSERT,
	INSERTDIR,
	REMOVE,
	HISTORY,
	SEARCH,
	SEARCHBYNAME,
	CLEARHISTORY,
	GETFILESFROMDIR,
	GETALLFILES,
};

struct Message {
	MsgType type = MsgType::REQUEST;
	Command command = Command::NONE;
	FileInfo fi;
};