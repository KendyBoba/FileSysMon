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
		std::cout << "No result\n" << std::endl;
		return result;
	}
	if (p_msg->command == Command::_ERROR_) {
		std::cout << "Something went wrong" << std::endl;
		return result;
	}
	std::memset(region.get_address(), 0, this->shared_size);
	return result;
}

void ConsoleProgram::insert(const  userInput& uinp)
{
	if (uinp.empty()) {
		//browser
	}
	boost::filesystem::path p = uinp.find(L"-p").operator*().second;
	if (boost::filesystem::is_directory(p))
		req(Command::INSERTDIR, uinp.find(L"-p").operator*().second);
	else
		req(Command::INSERT, uinp.find(L"-p").operator*().second);
}

void ConsoleProgram::search(const  userInput& uinp)
{
	if (uinp.empty()) {
		std::cout << "minimum 1 argument\n";
		return;
	}
	boost::filesystem::path p = getValue(uinp,L"-p");
	if (uinp.count(L"-n")) {
		auto res = req(Command::SEARCHBYNAME, uinp.find(L"-n").operator*().second);
		print(*res);
	}
	else if (uinp.count(L"-a")) {
		auto res = req(Command::GETALLFILES, L"");
		print(*res);
	}
	else if(boost::filesystem::is_directory(p)) {
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
	/*if (uinp.args.empty()) {
		std::cout << "minimum 1 argument\n";
		return;
	}
	while (request.first->command != Command::NONE) {};
	boost::filesystem::path p = uinp.args[0];
	if (boost::filesystem::is_directory(p))
		return;
	request.first->command = Command::REMOVE;
	request.first->path = toByteArray<MAX_BYTE_PATH>(p.wstring());
	while (request.first->command != Command::NONE && request.first->command != Command::_ERROR_) {};
	if (request.first->command == Command::_ERROR_) {
		std::cout << "Something went wrong" << std::endl;
		return;
	}*/
}

void ConsoleProgram::history(const userInput& uinp)
{
	/*if (uinp.args.empty()) {
		std::cout << "minimum 1 argument\n";
		return;
	}
	boost::filesystem::path p = uinp.args[0];
	if (boost::filesystem::is_directory(p))
		return;
	while (request.first->command != Command::NONE) {};
	request.first->command = Command::HISTORY;
	request.first->path = toByteArray<MAX_BYTE_PATH>(p.wstring());
	while (request.first->command != Command::NONE && request.first->command != Command::_ERROR_) {};
	if (request.first->command == Command::_ERROR_) {
		std::cout << "Something went wrong" << std::endl;
		return;
	}
	if (response.first->empty()) {
		std::wcout << L"no result\n";
		return;
	}
	if (response.first->size() == 1 && !response.first->front().file_path.len) {
		std::wcout << L"history is empty" << std::endl;
		return;
	}
	for (const FileInfo& el : *response.first) {
		print(el);
	}*/
}

void ConsoleProgram::clearHistory(const  userInput& uinp)
{
	//while (request.first->command != Command::NONE) {};
	//if (uinp.args.empty()) {
	//	request.first->command = Command::CLEARHISTORY;
	//}
	//else {
	//	boost::filesystem::path p = uinp.args[0];
	//	if (boost::filesystem::is_directory(p))
	//		return;
	//	request.first->command = Command::CLEARHISTORY;
	//	request.first->path = toByteArray<MAX_BYTE_PATH>(uinp.args[0]);
	//}
	//while (request.first->command != Command::NONE && request.first->command != Command::_ERROR_) {};
	//if (request.first->command == Command::_ERROR_) {
	//	std::cout << "Something went wrong" << std::endl;
	//	return;
	//}
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
	shared_mutex = OpenMutex(MUTEX_ALL_ACCESS, true, shared_mutex_name.c_str());
	if (shared_mutex == INVALID_HANDLE_VALUE)
		throw std::runtime_error("eror event create");
	WaitForSingleObject(shared_mutex,INFINITE);
	share_obj = boost::interprocess::shared_memory_object(boost::interprocess::open_only, name_of_shared_mem.c_str(), boost::interprocess::read_write);
	region = boost::interprocess::mapped_region(share_obj, boost::interprocess::read_write);
	p_msg = (Message*)region.get_address();
	ReleaseMutex(shared_mutex);
	functions = std::make_unique<cpmap>();
	functions->insert(std::make_pair(L"insert", std::bind(&ConsoleProgram::insert, this, std::placeholders::_1)));
	functions->insert(std::make_pair(L"search", std::bind(&ConsoleProgram::search, this, std::placeholders::_1)));
	/*functions->insert(std::make_pair(L"remove", std::bind(&ConsoleProgram::remove, this, std::placeholders::_1)));
	functions->insert(std::make_pair(L"history", std::bind(&ConsoleProgram::history, this, std::placeholders::_1)));
	functions->insert(std::make_pair(L"clearhistory", std::bind(&ConsoleProgram::clearHistory, this, std::placeholders::_1)));*/
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
		boost::to_lower(args.front());
		std::wstring command = args.front();
		args.erase(args.begin());
		for (size_t i = 0; i < args.size();++i) {
			if (args[i][0] != '-')
				continue;
			if (i + 1 < args.size())
				uinp[args[i]] = args[i + 1];
			else
				uinp[args[i]] = std::wstring(L"");
		}
		(functions->find(command).operator*().second)(uinp);
	}
}
