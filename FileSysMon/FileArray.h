#pragma once
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/optional.hpp>
#include <fstream>
#include <memory>
#include <set>

template<class MetaData,class Object>
class FileArray {
private:
	size_t size = 0;
	std::unique_ptr<std::fstream> file;
	static std::set<boost::filesystem::path>& all_opened_file() {
		static std::set<boost::filesystem::path> aof;
		return aof;
	}
public:
	boost::filesystem::path file_path;
	static const size_t init_size = 8;
public:
	FileArray() {};
	FileArray(const boost::filesystem::path& path);
	FileArray(const boost::filesystem::path& path,size_t start_size);
	FileArray(FileArray&& copy);
	~FileArray();
	FileArray(const FileArray& copy) = delete;
	FileArray& operator=(FileArray&& copy);
	FileArray& operator=(const FileArray & copy) = delete;
	void open(const boost::filesystem::path& path, size_t start_size);
	boost::optional<Object> read(size_t index)const;
	void write(const Object obj, size_t index);
	void resize(const size_t new_size);
	boost::optional<MetaData> getMD()const;
	void setMD(const MetaData& md);
	size_t getSize()const;
};

template<class MetaData, class Object>
FileArray<MetaData, Object>::FileArray(const boost::filesystem::path& path) : FileArray(path, init_size) {}


template<class MetaData, class Object>
FileArray<MetaData, Object>::FileArray(const boost::filesystem::path& path, size_t start_size) {
	open(path, start_size);
}

template<class MetaData, class Object>
FileArray<MetaData, Object>::FileArray(FileArray&& copy)
	: size(copy.size)
{
	copy.size = 0;
	file = std::move(copy.file);
	file_path = std::move(copy.file_path);
}

template<class MetaData, class Object>
FileArray<MetaData,Object>& FileArray<MetaData, Object>::operator=(FileArray&& copy) {
	if (this == &copy)
		return *this;
	size = copy.size;
	file_path = std::move(copy.file_path);
	file = std::move(copy.file_path);
	copy.size = 0;
	return *this;
}

template<class MetaData, class Object>
FileArray<MetaData, Object>::~FileArray() {
	file->seekp(0, std::fstream::beg);
	file->write((char*)&size, sizeof(size_t));
	file->close();
	all_opened_file().erase(this->file_path);
}

template<class MetaData, class Object>
void FileArray<MetaData, Object>::open(const boost::filesystem::path& path, size_t start_size) {
	if (FileArray::all_opened_file().count(path)){
		throw std::ios_base::failure("file was already opened");
	}
	if (!boost::filesystem::exists(path)) {
		std::ofstream temp(path.wstring(), std::ios::binary);
		temp.close();
	}
	this->file_path = path;
	if(!file)
		file = std::make_unique<std::fstream>(path.wstring(), std::fstream::in | std::fstream::out | std::fstream::binary);
	else {
		file->close();
		file->open(path.wstring(), std::fstream::in | std::fstream::out | std::fstream::binary);
	}
	if (!file->is_open()) {
		file->close();
		file.reset();
		throw std::ios_base::failure("file was not opened");
	}
	if(boost::filesystem::file_size(path))
		file->read((char*)&this->size, sizeof(size_t));
	else {
		this->size = 0;
		this->resize(start_size);
	}
	FileArray::all_opened_file().insert(path);
}

template<class MetaData, class Object>
boost::optional<Object> FileArray<MetaData,Object>::read(size_t index)const {
	boost::optional<Object> res;
	if (!file)
		return res;
	file->seekp(sizeof(size_t) + sizeof(MetaData) + sizeof(Object) * index, std::fstream::beg);
	Object obj;
	file->read((char*)&obj, sizeof(Object));
	res = obj;
	return res;
}
template<class MetaData, class Object>
void FileArray<MetaData, Object>::write(const Object obj,size_t index) {
	if (!file)
		return;
	file->seekp(sizeof(size_t) + sizeof(MetaData) + sizeof(Object) * index, std::fstream::beg);
	file->write((char*)&obj, sizeof(Object));
}

template<class MetaData, class Object>
void FileArray<MetaData, Object>::resize(const size_t new_size) {
	if (!boost::filesystem::exists(file_path) || !file)
		return;
	boost::filesystem::resize_file(file_path, sizeof(size_t) + sizeof(MetaData) + sizeof(Object) * new_size);
	size_t prev_size = size;
	size = new_size;
	file->seekp(0, std::fstream::beg);
	file->write((char*)&this->size, sizeof(size_t));
	Object null_obj;
	for (size_t i = prev_size; i < new_size; ++i)
		this->write(null_obj, i);
}

template<class MetaData, class Object>
boost::optional<MetaData> FileArray<MetaData, Object>::getMD()const {
	boost::optional<MetaData> res;
	if (!file)
		return res;
	file->seekp(sizeof(size_t),std::fstream::beg);
	res = MetaData();
	file->read((char*)&res.get(), sizeof(MetaData));
	return res;
}

template<class MetaData, class Object>
void FileArray<MetaData, Object>::setMD(const MetaData& md) {
	if (!file)
		return;
	file->seekp(sizeof(size_t), std::fstream::beg);
	file->write((char*)&md, sizeof(MetaData));
}

template<class MetaData, class Object>
size_t FileArray<MetaData, Object>::getSize()const {
	return this->size;
}