#pragma once
#include "FileArray.h"
#include <list>

template<class Object>
class FileDynamicArray {
private:
	struct StorageElem
	{
		Object obj = Object();
		long long ref = -1;
		long long revers_ref = -1; //TODO
	};
private:
	const size_t resize_size = 16;
	size_t len = 0;
	std::unique_ptr<FileArray<size_t, StorageElem>> fa = nullptr;
public:
	FileDynamicArray(){};
	FileDynamicArray(boost::filesystem::path& path);
	FileDynamicArray(const FileDynamicArray& obj) = delete;
	FileDynamicArray(FileDynamicArray&& obj);
	FileDynamicArray& operator=(const FileDynamicArray& obj) = delete;
	FileDynamicArray& operator=(FileDynamicArray&& obj);
	void open(boost::filesystem::path& path);
	void toTie(const size_t first,const size_t second);
	std::shared_ptr<std::list<size_t>> getTies(const size_t start_index);
	void removeTie(const size_t index);
	void push_back(const Object& obj);
	void remove(const size_t index);
	void pop_back();
	void run(std::function<void(const Object obj)> call_back);
	size_t getLen()const;
	Object get(size_t index);
	void set(const Object& obj, size_t index);
private:
	void resize();
};

template<class Object>
FileDynamicArray<Object>::FileDynamicArray(boost::filesystem::path& path) {
	open(path);
}

template<class Object>
FileDynamicArray<Object>::FileDynamicArray(FileDynamicArray&& obj) {
	this->len = obj.len;
	obj.len = 0;
	fa = std::move(obj.fa);
}

template<class Object>
FileDynamicArray<Object>& FileDynamicArray<Object>::operator=(FileDynamicArray&& obj) {
	if (this == &obj)
		return *this;
	len = obj.len;
	obj.len = 0;
	fa = std::move(obj.fa);
	return *this;
}

template<class Object>
void FileDynamicArray<Object>::open(boost::filesystem::path& path) {
	if (!fa) {
		fa = std::make_unique<FileArray<size_t, StorageElem>>(path);
	}
	else {
		fa->open(path, FileArray<size_t, StorageElem>::init_size);
	}
	this->len = (fa->getMD()) ? fa->getMD().value() : len;
}

template<class Object>
void FileDynamicArray<Object>::resize() {
	if (len + 1 >= fa->getSize())
		fa->resize(fa->getSize() + resize_size);
	else if (len + 1 + ( 2 * resize_size ) < fa->getSize()) {
		fa->resize(fa->getSize() - resize_size);
	}
	fa->setMD(len);
}

template<class Object>
void FileDynamicArray<Object>::push_back(const Object& obj) {
	fa->write(StorageElem{obj,-1,-1}, len++);
	resize();
}


template<class Object>
void FileDynamicArray<Object>::pop_back() {
	--len;
	resize();
}

template<class Object>
void FileDynamicArray<Object>::toTie(const size_t first, const size_t second) {
	auto opt = fa->read(first);
	auto sec_opt = fa->read(second);
	if (!opt || ! sec_opt)
		return;
	StorageElem el = opt.value();
	StorageElem sec_el = sec_opt.value();
	el.ref = second;
	sec_el.revers_ref = first;
	fa->write(el, first);
	fa->write(sec_el, second);
}

template<class Object>
void FileDynamicArray<Object>::removeTie(const size_t index) {
	auto opt = fa->read(index);
	if (!opt)
		return;
	StorageElem el = opt.value();
	auto sec_opt = fa->read(el.ref);
	if (!sec_opt)
		return;
	StorageElem sec_el = sec_opt.value();
	sec_el.ref = -1;
	fa->write(sec_el, el.ref);
	el.ref = -1;
	fa->write(el, index);
}

template<class Object>
void FileDynamicArray<Object>::remove(const size_t index) {
	auto get = [&](const size_t index)->StorageElem {
		auto opt = fa->read(index);
		if(!opt)
			throw std::runtime_error("storage is broke");
		return opt.value();
	};
	StorageElem delete_obj = get(index);
	if (delete_obj.revers_ref >= 0) {
		StorageElem prev = get(delete_obj.revers_ref);
		prev.ref = delete_obj.ref;
		fa->write(prev, delete_obj.revers_ref);
		if (delete_obj.ref >= 0) {
			StorageElem next = get(delete_obj.ref);
			next.revers_ref = delete_obj.revers_ref;
			fa->write(next, delete_obj.ref);
		}
	}
	for (size_t i = index; i < fa->getSize() - 1; ++i) {
		StorageElem next = get(i + 1);
		if (next.revers_ref >= 0) {
			StorageElem prev = get(next.revers_ref);
			--prev.ref;
			fa->write(prev, next.revers_ref);
		}
		if (next.ref >= 0)
			--next.ref;
		fa->write(next, i);
	}
	--len;
	resize();
}

template<class Object>
void FileDynamicArray<Object>::run(std::function<void(const Object obj)> call_back) {
	for (size_t i = 0; i < this->len; ++i) {
		auto opt = fa->read(i);
		if (!opt)
			continue;
		call_back(opt.value().obj);
	}
}

template<class Object>
std::shared_ptr<std::list<size_t>> FileDynamicArray<Object>::getTies(const size_t start_index) {
	auto result = std::make_shared<std::list<size_t>>();
	size_t i = start_index;
	do {
		result->push_front(i);
		auto opt = fa->read(i);
		if (!opt)
			continue;
		StorageElem el = opt.value();
		i = el.ref;
	} while (i >= 0 && i < this->len);
	return result;
}

template<class Object>
size_t FileDynamicArray<Object>::getLen()const {
	return this->len;
}

template<class Object>
Object FileDynamicArray<Object>::get(size_t index) {
	auto opt = fa->read(index);
	if (!opt)
		throw std::out_of_range("out of range - FileDyamicArray");
	return opt.value().obj;
}

template<class Object>
void  FileDynamicArray<Object>::set(const Object& obj, size_t index) {
	if(index >= len)
		throw std::out_of_range("out of range - FileDyamicArray");
	auto opt = fa->read(index);
	if (!opt)
		return;
	StorageElem el = opt.value();
	fa->write(StorageElem{obj,el.ref,el.revers_ref});
}