#include "CheckDiff.h"

CheckDiff::CheckDiff()
{
	init();
}

CheckDiff::~CheckDiff()
{
	boost::interprocess::shared_memory_object::remove(shared_memory_name.c_str());
	CloseHandle(shared_mutex);
}

void CheckDiff::init()
{
	SECURITY_ATTRIBUTES sa = this->makeSA();
	shared_mutex = CreateMutex(&sa, true, shared_mutex_name.c_str());
	if(shared_mutex == NULL)
		throw std::runtime_error("eror mutex create");
	this->dirs = std::make_unique<std::set<std::wstring>>();
	auto p_all_dir = Storage::instance().getAllDir();
	for (const std::wstring& path_to_dir : *p_all_dir) {
		this->slotAdd(path_to_dir);
		dirs->insert(path_to_dir);
	}
}

void CheckDiff::readChanges(const boost::filesystem::path& path)
{
	HANDLE dir = CreateFileW(
		path.wstring().c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		NULL);
	const unsigned short max_buf_size = 1024;
	char buff[max_buf_size];
	ZeroMemory(buff, max_buf_size);
	while (dirs->count(path.wstring()) && global::instance().is_work) {
		while (global::instance().is_pause) { std::this_thread::sleep_for(std::chrono::milliseconds(200)); };
		OVERLAPPED overlaped;
		overlaped.hEvent = CreateEvent(NULL, false, false, NULL);
		ReadDirectoryChangesW(
			dir,
			buff,
			max_buf_size,
			true,
			FILE_NOTIFY_CHANGE_FILE_NAME
			| FILE_NOTIFY_CHANGE_DIR_NAME
			| FILE_NOTIFY_CHANGE_ATTRIBUTES
			| FILE_NOTIFY_CHANGE_SIZE
			| FILE_NOTIFY_CHANGE_LAST_WRITE
			| FILE_NOTIFY_CHANGE_LAST_ACCESS
			| FILE_NOTIFY_CHANGE_CREATION
			| FILE_NOTIFY_CHANGE_SECURITY,
			NULL,
			&overlaped,
			NULL);
		size_t pos = 0;
		FILE_NOTIFY_INFORMATION* fni = nullptr;
		do {
			fni = (FILE_NOTIFY_INFORMATION*)&buff[pos];
			if (!fni->FileNameLength)
				break;
			std::wstring file_path = path.wstring();
			file_path += std::wstring(fni->FileName, fni->FileNameLength / sizeof(wchar_t));
			auto result = make_info(file_path);
			if (!result) {
				ZeroMemory(buff, max_buf_size);
				continue;
			}
			result->change = (FileInfo::Changes)fni->Action;
			this->storage_mutex.lock();
			this->procChange(result);
			this->storage_mutex.unlock();
			pos = fni->NextEntryOffset;
		} while (fni->NextEntryOffset);
		ZeroMemory(buff, max_buf_size);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	CloseHandle(dir);
}

void CheckDiff::procChange(std::shared_ptr<FileInfo> fi)
{
	if (!Storage::instance().isfileExist(*fi)) {
		return;
	}
	Storage::instance().addToHistory(*fi);
	if (!use_log)
		return;
	writeLog(fi);
}

SECURITY_ATTRIBUTES CheckDiff::makeSA() const
{
	SECURITY_ATTRIBUTES result{0};
	PSID users = nullptr;
	SID_IDENTIFIER_AUTHORITY authWorld = SECURITY_WORLD_SID_AUTHORITY;
	if (!AllocateAndInitializeSid(&authWorld,1,SECURITY_WORLD_RID,0,0,0,0,0,0,0,&users))
		throw std::runtime_error("Attribute creation error (sid)");
	EXPLICIT_ACCESS ea[1];
	ZeroMemory(&ea, sizeof(ea));
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[0].Trustee.ptstrName = (LPWCH)users;
	PACL acl = nullptr;
	if(SetEntriesInAcl(1, ea, nullptr, &acl) != ERROR_SUCCESS)
		throw std::runtime_error("Attribute creation error (acl)");
	PSECURITY_DESCRIPTOR psd = (PSECURITY_DESCRIPTOR*)LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if(!psd)
		throw std::runtime_error("Attribute creation error (psd)");
	InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);
	if(!SetSecurityDescriptorDacl(psd,true,acl,false))
		throw std::runtime_error("Attribute creation error (set psd)");
	result.nLength = sizeof(SECURITY_ATTRIBUTES);
	result.lpSecurityDescriptor = psd;
	result.bInheritHandle = false;
	return result;
}

void CheckDiff::slotAdd(const std::wstring& dir_path)
{
	if (dirs->count(dir_path))
		return;
	dirs->insert(dir_path);
	auto th_func = std::bind(&CheckDiff::readChanges, this, boost::filesystem::path(dir_path));
	std::thread th(th_func);
	th.detach();
}

void CheckDiff::slotRemove(const std::wstring& dir_path)
{
	dirs->erase(dir_path);
}

void CheckDiff::setLog(std::shared_ptr<std::fstream> error_log, std::shared_ptr<std::fstream> change_log)
{
	this->error_log = error_log;
	this->change_log = change_log;
	use_log = true;
}

void CheckDiff::writeLog(std::shared_ptr<FileInfo> fi)
{
	if (!use_log)
		return;
	std::wstring path = fromByteArray(fi->file_path);
	std::wstring str_creation_time = timeToStr(systemTimeToTm(fi->creation_time));
	std::wstring str_last_access_time = timeToStr(systemTimeToTm(fi->last_access_time));
	std::wstring str_last_write_time = timeToStr(systemTimeToTm(fi->last_write_time));
	std::wstring str_change_time = timeToStr(fi->change_time);
	json::object jobj;
	jobj.emplace("path", toUTF8(path));
	jobj.emplace("file size", fi->file_size);
	jobj.emplace("attribute", fi->attribute);
	jobj.emplace("links", fi->num_of_links);
	jobj.emplace("creation time", toUTF8(str_creation_time));
	jobj.emplace("last access time", toUTF8(str_last_access_time));
	jobj.emplace("last write time", toUTF8(str_last_write_time));
	jobj.emplace("change", (int)fi->change);
	jobj.emplace("change time", toUTF8(str_change_time));
	json::value jv(jobj);
	std::string log_str = json::serialize(jv);
	*this->change_log << log_str << std::endl;
	change_log->flush();
}

void CheckDiff::exec()
{
	SECURITY_ATTRIBUTES sec_attr = this->makeSA();
	HANDLE share_obj = CreateFileMapping(NULL, &sec_attr, PAGE_READWRITE,0, shared_size, shared_memory_name.c_str());
	if (share_obj == nullptr)
		throw std::runtime_error("Failure to open shared object");
	Message* p_msg = (Message*)MapViewOfFile(share_obj, FILE_MAP_ALL_ACCESS, 0, 0, shared_size);
	if(!p_msg)
		throw std::runtime_error("Failure to open view object");
	std::memset(p_msg, 0, this->shared_size);
	ReleaseMutex(shared_mutex);
	auto write = [&](Command command,FileInfo&& fi)->void {
		WaitForSingleObject(shared_mutex, INFINITE);
		p_msg->type = MsgType::RESPONSE;
		p_msg->command = command;
		p_msg->fi = fi;
		ReleaseMutex(shared_mutex);
	};
	auto read = [&]()->Message {
		WaitForSingleObject(shared_mutex, INFINITE);
		Message res = *p_msg;
		ReleaseMutex(shared_mutex);
		return res;
	};
	auto send = [write,read](Command command, FileInfo&& fi)->void {
		while (read().type == MsgType::RESPONSE) { std::this_thread::sleep_for(std::chrono::microseconds(10)); };
		write(command, std::move(fi));
	};
	try {
		Message msg;
		while (global::instance().is_work) {
			msg = read();
			*error_log << (int)msg.command << std::endl; error_log->flush();
			global::instance().UpdateStatus(SERVICE_RUNNING);
			while (global::instance().is_pause) { std::this_thread::sleep_for(std::chrono::milliseconds(200)); };
			switch (msg.command)
			{
			case Command::INSERT: {
				auto pfi = make_info(fromByteArray(msg.fi.file_path));
				if(!pfi)
					send(Command::_ERROR_BAD_ARG_, FileInfo());
				this->storage_mutex.lock();
				Storage::instance().insert(*pfi);
				this->storage_mutex.unlock();
				send(Command::NONE, FileInfo());
			}; break;
			case Command::INSERTDIR: {
				this->storage_mutex.lock();
				Storage::instance().insert(fromByteArray(msg.fi.file_path));
				this->storage_mutex.unlock();
				send(Command::NONE, FileInfo());
			}; break;
			case Command::REMOVE: {
				this->storage_mutex.lock();
				Storage::instance().remove(fromByteArray(msg.fi.file_path));
				this->storage_mutex.unlock();
				send(Command::NONE, FileInfo());
			}; break;
			case Command::HISTORY: {
				this->storage_mutex.lock();
				auto list_of_history = Storage::instance().getHistory(fromByteArray<MAX_BYTE_PATH>(msg.fi.file_path));
				this->storage_mutex.unlock();
				for (auto& el : *list_of_history) {
					send(Command::CONTINUE, std::move(el));
				}
				send(Command::NONE, FileInfo());
			}; break;
			case Command::SEARCH: {
				this->storage_mutex.lock();
				auto pFi = Storage::instance().searchByPath(fromByteArray(msg.fi.file_path));
				this->storage_mutex.unlock();
				if (pFi)
					send(Command::NONE, std::move(*pFi));
				else
					send(Command::NONE, FileInfo());
			}; break;
			case Command::CLEARHISTORY: {
				this->storage_mutex.lock();
				if (!msg.fi.file_path.len)
					Storage::instance().clearHistory();
				else
					Storage::instance().clearHistory(fromByteArray(msg.fi.file_path));
				this->storage_mutex.unlock();
				send(Command::NONE, FileInfo());
			}; break;
			case Command::GETFILESFROMDIR: {
				this->storage_mutex.lock();
				auto p_files = Storage::instance().getFilesFromDir(fromByteArray(msg.fi.file_path));
				this->storage_mutex.unlock();
				for (FileInfo& el : *p_files) {
					send(Command::CONTINUE, std::move(el));
				}
				send(Command::NONE, FileInfo());
			}; break;
			case Command::GETALLFILES: {
				this->storage_mutex.lock();
				auto p_files = Storage::instance().getAllFiles();
				this->storage_mutex.unlock();
				for (FileInfo& fi : *p_files) {
					send(Command::CONTINUE, std::move(fi));
				}
				send(Command::NONE, FileInfo());
			}; break;
			case Command::SEARCHBYNAME: {
				this->storage_mutex.lock();
				auto p_files = Storage::instance().searchByName(fromByteArray<MAX_BYTE_PATH>(msg.fi.file_path));
				this->storage_mutex.unlock();
				for (FileInfo& fi : *p_files) {
					send(Command::CONTINUE, std::move(fi));
				}
				send(Command::NONE, FileInfo());
			}; break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
#ifdef _DEBUG
			this->storage_mutex.lock();
			Storage::instance().print();
			this->storage_mutex.unlock();
#endif
		}
		global::instance().UpdateStatus(SERVICE_STOP_PENDING);
	}
	catch (...) {
		UnmapViewOfFile(p_msg);
		CloseHandle(share_obj);
		write(Command::_ERROR_, FileInfo());
		throw;
	}
	UnmapViewOfFile(p_msg);
	CloseHandle(share_obj);
}
