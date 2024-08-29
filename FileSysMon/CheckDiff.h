#pragma once
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include "Storage.h"
#include "Message.h"
#include <set>
#include <mutex>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#define BOOST_JSON_STACK_BUFFER_SIZE 256
#include <boost/json.hpp>
namespace json = boost::json;
class CheckDiff {
public:
	enum status {
		STOP,
		WORK,
	};
private:
	std::unique_ptr<std::set<std::wstring>> dirs = nullptr;
	const std::string shared_memory_name = "FileSysMonShareMem";
	const std::wstring shared_mutex_name = L"FileSysMonMutex";
	HANDLE shared_mutex = NULL;
	const unsigned short shared_size = 2048;
	std::mutex storage_mutex;
	std::shared_ptr<std::fstream> error_log = nullptr;
	std::shared_ptr<std::fstream> change_log = nullptr;
	bool use_log = false;
	bool is_work;
	bool is_pause;
public:
	CheckDiff(bool& is_work, bool& is_pause);
	~CheckDiff();
	void init();
	void readChanges(const boost::filesystem::path& path);
	void procChange(std::shared_ptr<FileInfo> fi);
	void slotAdd(const std::wstring& dir_path);
	void slotRemove(const std::wstring& dir_path);
	void setLog(std::shared_ptr<std::fstream> error_log, std::shared_ptr<std::fstream> change_log);
	void writeLog(std::shared_ptr<FileInfo> fi);
	void exec(std::function<void(int)> call_back);
};