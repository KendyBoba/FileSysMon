#pragma once
#include "FileArray.h"
#include <list>

template<class Key,class Value>
class FileMap {
private:
	struct Cell
	{
		Key key = Key();
		Value val = Value();
		bool empty = true;
	};
private:
	size_t fill = 0;
	std::unique_ptr<FileArray<size_t, Cell>> arr = nullptr;
public:
	FileMap() {};
	FileMap(const boost::filesystem::path& path);
	FileMap(FileMap&& fa);
	FileMap& operator=(FileMap&& fa);
	FileMap(const FileMap& copy) = delete;
	FileMap& operator=(const FileMap& copy) = delete;
	void insert(const Key& key, const Value& value);
	boost::optional<Value> search(const Key& key);
	bool changeValue(const Key& key, const Value& new_value);
	void open(const boost::filesystem::path& path);
	void remove(const Key& key);
	void run(std::function<void(const Key, const Value)> call_back);
	bool contains(const Key& key);
private:
	bool insert(std::unique_ptr<FileArray<size_t,Cell>>& dist,const Key& key,const Value& val);
	boost::optional<size_t> find(std::unique_ptr<FileArray<size_t, Cell>>& dist, const Key& key);
	double factor();
	void resize();
	size_t hash(const Key& key);
};

template<class Key, class Value>
FileMap<Key, Value>::FileMap(const boost::filesystem::path& path){
	this->open(path);
}

template<class Key, class Value>
FileMap<Key, Value>::FileMap(FileMap&& fa){
	fill = fa.fill;
	fa.fill = 0;
	arr = std::move(fa.arr);
}

template<class Key, class Value>
FileMap<Key,Value>& FileMap<Key, Value>::operator=(FileMap&& fa) {
	if (this == &fa)
		return *this;
	fill = fa.fill;
	arr = std::move(fa.arr);
	fa.fill = 0;
	return *this;
}

template<class Key, class Value>
void FileMap<Key, Value>::insert(const Key& key, const Value& value) {
	if(this->insert(arr, key, value))
		++this->fill;
	this->resize();
}

template<class Key, class Value>
bool FileMap<Key, Value>::insert(std::unique_ptr<FileArray<size_t, Cell>>& dist, const Key& key, const Value& val) {
	size_t pos = this->hash(key);
	size_t step = pos * 2 + 1;
	pos = pos % dist->getSize();
	boost::optional<Cell> opt;
	while ((opt = dist->read(pos)).value().empty == false) {
		if (opt.value().key == key)
			return false;
		pos = (pos + step) % dist->getSize();
	}
	dist->write(Cell{ key,val,false }, pos);
	return true;
}

template<class Key, class Value>
void FileMap<Key, Value>::open(const boost::filesystem::path& path) {
	if (!arr)
		arr = std::make_unique<FileArray<size_t, Cell>>(path);
	else {
		arr->open(path, FileArray<size_t, Cell>::init_size);
	}
	this->fill = (arr->getMD()) ? arr->getMD().value() : fill;
}

template<class Key, class Value>
boost::optional<Value> FileMap<Key, Value>::search(const Key& key){
	boost::optional<Value> res;
	auto opt_pos = this->find(arr, key);
	if (!opt_pos)
		return res;
	auto opt_elem_into_pos = arr->read(opt_pos.value());
	if (!opt_elem_into_pos)
		return res;
	res = opt_elem_into_pos.value().val;
	return res;
}

template<class Key, class Value>
bool FileMap<Key, Value>::changeValue(const Key& key, const Value& new_value) {
	auto opt_pos = this->find(arr, key);
	if (!opt_pos)
		return false;
	auto opt_elem_into_pos = arr->read(opt_pos.value());
	if (!opt_elem_into_pos)
		return false;
	arr->write(Cell{ key,new_value,false }, opt_pos.value());
	return true;
}

template<class Key, class Value>
boost::optional<size_t> FileMap<Key, Value>::find(std::unique_ptr<FileArray<size_t, Cell>>& dist, const Key& key){
	boost::optional<size_t> res;
	size_t pos = this->hash(key);
	size_t step = pos * 2 + 1;
	pos = pos % dist->getSize();
	size_t beg_pos = pos;
	do {
		auto opt = dist->read(pos);
		if (!opt) {
			pos = (pos + step) % dist->getSize();
			continue;
		}
		if (opt.value().key == key && !opt.value().empty) {
			res = pos;
			return res;
		}
		pos = (pos + step) % dist->getSize();
	} while (beg_pos != pos);
	return res;
}

template<class Key, class Value>
double FileMap<Key, Value>::factor() {
	return (double)fill / (double)arr->getSize();
}

template<class Key, class Value>
void FileMap<Key, Value>::resize() {
	bool more = false;
	if ((more = factor() >= 0.75) || factor() <= 0.25) {
		this->fill = 0;
		boost::filesystem::path old_path = arr->file_path;
		boost::filesystem::path new_path = old_path;
		new_path += ".temp";
		size_t new_size = (more) ? arr->getSize() << 1 : arr->getSize() >> 1;
		auto new_arr = std::make_unique<FileArray<size_t, Cell>>(new_path, new_size);
		for (size_t i = 0; i < arr->getSize(); ++i) {
			Cell el = arr->read(i).value();
			if (el.empty)
				continue;
			this->insert(new_arr, el.key, el.val);
			++this->fill;
		}
		new_arr->setMD(fill);
		new_arr.reset();
		arr.reset();
		boost::filesystem::remove(old_path);
		boost::filesystem::rename(new_path, old_path);
		this->open(old_path);
	}
	else {
		arr->setMD(fill);
	}
}

template<class Key, class Value>
size_t FileMap<Key, Value>::hash(const Key& key) {
	std::string buff = "";
	for (size_t i = 0; i < sizeof(key); ++i)
		buff += ((char*)&key)[i];
	return std::hash<std::string>{}(buff);
}

template<class Key, class Value>
void FileMap<Key, Value>::remove(const Key& key) {
	auto opt_pos = find(arr,key);
	if (!opt_pos)
		return;
	auto opt_elem_into_pos = arr->read(opt_pos.value());
	if (!opt_elem_into_pos)
		return;
	--fill;
	arr->write(Cell{ key,opt_elem_into_pos.value().val,true}, opt_pos.value());
	this->resize();
}

template<class Key, class Value>
void FileMap<Key, Value>::run(std::function<void(const Key, const Value)> call_back) {
	for (size_t i = 0; i < arr->getSize(); ++i) {
		auto opt_el = arr->read(i);
		if (!opt_el)
			continue;
		if (opt_el.value().empty)
			continue;
		call_back(opt_el.value().key, opt_el.value().val);
	}
}

template<class Key, class Value>
bool FileMap<Key, Value>::contains(const Key& key){
	auto opt_pos = this->find(arr, key);
	if (!opt_pos)
		return false;
	return true;
}