#pragma once
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <vector>
#include "Message.h"
#include <boost/filesystem.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/algorithm/string.hpp>
class ConsoleProgram {
private:
	using userInput = std::map<std::wstring, std::wstring>;
	HANDLE shar_mutex = NULL;
	using cpmap = std::map<std::wstring, std::function<void(const std::map<std::wstring, std::wstring>&)>>;
	const std::string name_of_shared_mem = "FileSysMonShareMem";
	std::unique_ptr<cpmap> functions = nullptr;
	const std::wstring shared_mutex_name = L"FileSysMonMutex";
	HANDLE shared_mutex = NULL;
	boost::interprocess::shared_memory_object share_obj;
	boost::interprocess::mapped_region region;
	const unsigned short shared_size = 2048;
	Message* p_msg = nullptr;
private:
	std::shared_ptr<std::vector<FileInfo>> req(Command command, const std::wstring& arg);
	void insert(const userInput& uinp);
	void search(const userInput& uinp);
	void remove(const userInput& uinp);
	void history(const userInput& uinp);
	void clearHistory(const userInput& uinp);
	std::wstring getValue(const userInput& uinp, const std::wstring& key)const;
	void write(Command cmd, const FileInfo& fi);
	Message read();
	void send(Command cmd, const FileInfo& fi);
public:
	static void print(const FileInfo& fi);
	static void print(const std::vector<FileInfo>& vfi);
public:
	ConsoleProgram();
	void exec();
};