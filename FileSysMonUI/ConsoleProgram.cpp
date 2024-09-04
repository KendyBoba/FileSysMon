#include "ConsoleProgram.h"

std::shared_ptr<std::vector<FileInfo>> ConsoleProgram::req(Command command, const std::wstring& arg)
{
	auto result = std::make_shared<std::vector<FileInfo>>();
	FileInfo fi_arg;
	fi_arg.file_path = toByteArray<MAX_BYTE_PATH>(arg);
	Message resp_msg;
	do {
		send(command, fi_arg);
		resp_msg = read();
		if (resp_msg.fi.file_path.len)
			result->push_back(resp_msg.fi);
	} while (resp_msg.command == Command::CONTINUE);
	if (result->empty()) {
		std::cout << "No result" << std::endl;
		return result;
	}
	if (p_msg->command == Command::_ERROR_) {
		std::cout << "Error: Something went wrong" << std::endl;
		return result;
	}
	if (p_msg->command == Command::_ERROR_BAD_ARG_) {
		std::cout << "Error: Bad argument" << std::endl;
		return result;
	}
	std::memset(p_msg, 0, this->shared_size);
	return result;
}

void ConsoleProgram::insert(const  userInput& uinp)
{
	if (uinp.empty()) {
		wchar_t* file_path = new wchar_t[MAX_PATH];
		LPOPENFILENAMEW lp_file_name = new OPENFILENAMEW{ 0 };
		lp_file_name->lStructSize = sizeof(OPENFILENAMEW);
		lp_file_name->hInstance = NULL;
		lp_file_name->lpstrFile = file_path;
		lp_file_name->lpstrFile[0] = '\0';
		lp_file_name->nMaxFile = MAX_PATH;
		lp_file_name->lpstrFilter = L"*.*";
		lp_file_name->lpstrInitialDir = L".\\";
		lp_file_name->lpstrTitle = L"Select file";
		if (!GetOpenFileName(lp_file_name)) {
			std::wcout << L"Failed to open file selection dialog\n";
			delete lp_file_name;
			delete file_path;
			return;
		}
		std::wstring path(file_path);
		req(Command::INSERT, path);
		delete lp_file_name;
		delete[] file_path;
		return;
	}
	boost::filesystem::path p = getValue(uinp, L"-p");
	if (boost::filesystem::is_directory(p))
		req(Command::INSERTDIR, p.wstring());
	else
		req(Command::INSERT, p.wstring());
}

void ConsoleProgram::search(const  userInput& uinp)
{
	if (uinp.empty()) {
		auto res = req(Command::GETALLFILES, L"");
		print(*res);
		return;
	}
	if (uinp.count(L"-n")) {
		auto res = req(Command::SEARCHBYNAME, getValue(uinp, L"-n"));
		print(*res);
		return;
	}
	boost::filesystem::path p = getValue(uinp,L"-p");
	if(boost::filesystem::is_directory(p)) {
		auto res = req(Command::GETFILESFROMDIR, p.wstring());
		print(*res);
	}
	else{
		auto res = req(Command::SEARCH, p.wstring());
		print(*res);
	}
}

void ConsoleProgram::remove(const  userInput& uinp)
{
	if (uinp.empty()) {
		auto res = req(Command::GETALLFILES, L"");
		for (const FileInfo& fi : *res) {
			req(Command::REMOVE, fromByteArray<MAX_BYTE_PATH>(fi.file_path));
		}
		return;
	}
	if (uinp.count(L"-n")) {
		auto res = req(Command::SEARCHBYNAME, getValue(uinp, L"-n"));
		for (const FileInfo& fi : *res) {
			req(Command::REMOVE, fromByteArray<MAX_BYTE_PATH>(fi.file_path));
		}
		return;
	}
	boost::filesystem::path p = getValue(uinp, L"-p");
	if (boost::filesystem::is_directory(p)) {
		auto res = req(Command::GETFILESFROMDIR, p.wstring());
		for (const FileInfo& fi : *res) {
			req(Command::REMOVE,fromByteArray<MAX_BYTE_PATH>(fi.file_path));
		}
	}
	else {
		req(Command::REMOVE, p.wstring());
	}
}

void ConsoleProgram::history(const userInput& uinp)
{
	if (uinp.empty()) {
		auto res = req(Command::GETALLFILES, L"");
		for (const FileInfo& fi : *res) {
			auto history_res = req(Command::HISTORY, fromByteArray<MAX_BYTE_PATH>(fi.file_path));
			print(*history_res);
		}
		return;
	}
	if (uinp.count(L"-n")) {
		auto res = req(Command::SEARCHBYNAME, getValue(uinp, L"-n"));
		for (const FileInfo& fi : *res) {
			req(Command::HISTORY, fromByteArray<MAX_BYTE_PATH>(fi.file_path));
		}
		return;
	}
	boost::filesystem::path p = getValue(uinp, L"-p");
	if (boost::filesystem::is_directory(p)) {
		auto res = req(Command::GETFILESFROMDIR, p.wstring());
		for (const FileInfo& fi : *res) {
			auto history_res = req(Command::HISTORY, fromByteArray<MAX_BYTE_PATH>(fi.file_path));
			print(*history_res);
		}
	}
	else {
		auto history_res = req(Command::HISTORY, p.wstring());
		print(*history_res);
	}
}

void ConsoleProgram::clearHistory(const  userInput& uinp)
{
	if (uinp.empty()) {
		req(Command::CLEARHISTORY, L"");
		return;
	}
	if (uinp.count(L"-n")) {
		auto res = req(Command::SEARCHBYNAME, getValue(uinp, L"-n"));
		for (const FileInfo& fi : *res) {
			req(Command::CLEARHISTORY, fromByteArray<MAX_BYTE_PATH>(fi.file_path));
		}
		return;
	}
	boost::filesystem::path p = getValue(uinp, L"-p");
	if (boost::filesystem::is_directory(p)) {
		auto res = req(Command::GETFILESFROMDIR, p.wstring());
		for (const FileInfo& fi : *res) {
			req(Command::CLEARHISTORY, fromByteArray<MAX_BYTE_PATH>(fi.file_path));
		}
	}
	else {
		req(Command::CLEARHISTORY, p.wstring());
	}
}

std::wstring ConsoleProgram::getValue(const userInput& uinp, const std::wstring& key) const
{
	auto it = uinp.find(key);
	if (it == uinp.end())
		return L"";
	return it.operator*().second;
}

void ConsoleProgram::write(Command cmd,const FileInfo & fi)
{
	WaitForSingleObject(shared_mutex, INFINITE);
	p_msg->command = cmd;
	p_msg->fi = fi;
	p_msg->type = MsgType::REQUEST;
	ReleaseMutex(shared_mutex);
}

Message ConsoleProgram::read()
{
	WaitForSingleObject(shared_mutex, INFINITE);
	Message res = *p_msg;
	ReleaseMutex(shared_mutex);
	return res;
}

void ConsoleProgram::send(Command cmd, const FileInfo& fi)
{
	write(cmd,fi);
	while (read().type == MsgType::REQUEST) { std::this_thread::sleep_for(std::chrono::microseconds(10)); };
}

void ConsoleProgram::print(const FileInfo& fi)
{
	if (!fi.file_path.len) {
		std::wcout << L"Invalid response\n";
	}
	std::wcout
		<< std::endl << L"File path: " << std::setw(16) << std::left << fromByteArray(fi.file_path)
		<< std::endl << L"File size: " << std::setw(16) << std::left << fi.file_size
		<< std::endl << L"Attribute: " << std::setw(16) << std::left << fi.attribute
		<< std::endl << L"Links: " << std::setw(16) << std::left << fi.num_of_links
		<< std::endl << L"Creation time: " << std::setw(16) << std::left << timeToStr(systemTimeToTm(fi.creation_time))
		<< std::endl << L"Last access time: " << std::setw(16) << std::left << timeToStr(systemTimeToTm(fi.last_access_time))
		<< std::endl << L"Last write time: " << std::setw(16) << std::left << timeToStr(systemTimeToTm(fi.last_write_time))
		<< std::endl;
	if (fi.change != FileInfo::Changes::EMPTY) {
		std::wcout
			<< L"Change: " << std::setw(16) << std::left << (int)fi.change
			<< std::endl << L"Change time: " << std::setw(16) << std::left << timeToStr(fi.change_time)
			<< std::endl;
	}
}

void ConsoleProgram::print(const std::vector<FileInfo>& vfi)
{
	for (const FileInfo& fi : vfi) {
		print(fi);
	}
}

ConsoleProgram::ConsoleProgram()
{
	while ((shared_mutex = OpenMutex(MUTEX_ALL_ACCESS, true, shared_mutex_name.c_str())) == NULL) {
		std::this_thread::sleep_for(std::chrono::microseconds(20));
	}
	WaitForSingleObject(shared_mutex, INFINITE);
	while ((this->share_obj = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, name_of_shared_mem.c_str())) == NULL) {
		std::this_thread::sleep_for(std::chrono::microseconds(20));
	}
	if (share_obj == nullptr)
		throw std::runtime_error("Failure to open shared object");
	this->p_msg = (Message*)MapViewOfFile(share_obj, FILE_MAP_ALL_ACCESS, 0, 0, shared_size);
	if (!p_msg)
		throw std::runtime_error("Failure to open view object");
	ReleaseMutex(shared_mutex);
	functions = std::make_unique<cpmap>();
	functions->insert(std::make_pair(L"insert", std::bind(&ConsoleProgram::insert, this, std::placeholders::_1)));
	functions->insert(std::make_pair(L"search", std::bind(&ConsoleProgram::search, this, std::placeholders::_1)));
	functions->insert(std::make_pair(L"remove", std::bind(&ConsoleProgram::remove, this, std::placeholders::_1)));
	functions->insert(std::make_pair(L"history", std::bind(&ConsoleProgram::history, this, std::placeholders::_1)));
	functions->insert(std::make_pair(L"clearhistory", std::bind(&ConsoleProgram::clearHistory, this, std::placeholders::_1)));
}

void ConsoleProgram::exec()
{
	std::wstring command_line;
	std::vector<std::wstring> split_vec;
	userInput uinp;
	std::vector<std::wstring> args;
	bool work = true;
	while (work) {
		split_vec.clear();
		uinp.clear();
		args.clear();
		command_line.clear();
		std::wcout << L"FSMU>> ";
		std::getline(std::wcin, command_line);
		boost::split(split_vec, command_line, boost::is_any_of(L" "));
		for (auto it = split_vec.begin(); it != split_vec.end(); ++it) {
			if (it->empty())
				break;
			if (it->front() == '\"') {
				it->erase(it->begin());
				std::wstring new_statement = L"";
				for (it; it != split_vec.end(); ++it) {
					if (it->back() == '\"') {
						it->pop_back();
						new_statement.append(*it);
						break;
					}
					*it += ' ';
					new_statement.append(*it);
				}
				args.push_back(new_statement);
			}
			else {
				args.push_back(*it);
			}
			if (it == split_vec.end())
				break;
		}
		if (args.empty())
			continue;
		boost::to_lower(args.front());
		std::wstring command = args.front();
		args.erase(args.begin());
		for (size_t i = 0; i < args.size();++i) {
			if (args[i][0] != '-')
				continue;
			boost::to_lower(args[i]);
			if (i + 1 < args.size())
				uinp[args[i]] = args[i + 1];
			else
				uinp[args[i]] = std::wstring(L"");
		}

		auto iter_cmd_func = functions->find(command);
		if (iter_cmd_func == functions->end()) {
			std::wcout << L"Unknown command\n";
			continue;
		}
		(*iter_cmd_func).second(uinp);
	}
}
