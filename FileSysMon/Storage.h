#pragma once
#include <iostream>
#include <set>
#include <map>
#include <stack>
#include "FileInfo.h"
#include "FileMap.h"
#include "FileDynamicArray.h"
#include <boost/filesystem.hpp>

class Storage {
public:  //signals
	std::function<void(const std::wstring&)> signalInsert;
	std::function<void(const std::wstring&)> signalRemove;
private:
	std::unique_ptr<FileMap<byte_arr<MAX_BYTE_PATH>, size_t>> index_path = nullptr;
	std::unique_ptr<FileDynamicArray<FileInfo>> data = nullptr;
	std::unique_ptr<FileDynamicArray<byte_arr<MAX_BYTE_PATH>>> data_dirs = nullptr;
	std::unique_ptr<FileMap<byte_arr<MAX_BYTE_PATH>, size_t>> index_dir = nullptr;
	std::unique_ptr<FileDynamicArray<size_t>> data_name = nullptr;
	std::unique_ptr<FileMap<byte_arr<MAX_BYTE_PATH>, size_t>> index_name = nullptr;
public:
	static Storage& instance();
	void insert(const FileInfo& fi);
	void insert(const std::wstring& dir_path);
	void remove(const std::wstring& path);
	std::shared_ptr<std::set<std::wstring>> getAllDir()const;
	std::shared_ptr<FileInfo> searchByPath(const std::wstring& path);
	std::shared_ptr<std::list<FileInfo>> searchByName(const std::wstring& name);
	std::shared_ptr<std::list<FileInfo>> getHistory(const std::wstring& path);
	std::shared_ptr<std::list<FileInfo>> getFilesFromDir(const std::wstring & dir_path);
	std::shared_ptr<std::list<FileInfo>> getAllFiles();
	void clearHistory();
	void clearHistory(const std::wstring& path);
	void addToHistory(FileInfo& fi);
	bool isfileExist(const FileInfo& fi);
private:
	Storage();
	std::shared_ptr<std::list<FileInfo>> fsGet(const boost::filesystem::path& dir_path);
	void clearDir(const std::wstring& path);
	void clearName(const std::wstring& path);
	void fixIndex(FileMap<byte_arr<MAX_BYTE_PATH>, size_t>& index, const size_t pos);
public:
	Storage(const Storage& obj) = delete;
	Storage(Storage&& obj) = delete;
	Storage& operator=(const Storage& obj) = delete;
	Storage& operator=(Storage&& obj) = delete;
#ifdef _DEBUG
public:
	void print()const;
#endif
};