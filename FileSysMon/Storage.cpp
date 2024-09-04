#include "Storage.h"

Storage& Storage::instance()
{
	static Storage* res = new Storage();
	return *res;
}

void Storage::insert(const FileInfo& fi)
{
	if (index_path->contains(fi.file_path)) {
		return;
	}
	data->push_back(fi);
	index_path->insert(fi.file_path, data->getLen() - 1);
	boost::filesystem::path path = fromByteArray(fi.file_path);
	std::wstring dir = path.parent_path().wstring() + L"\\";
	byte_arr<MAX_BYTE_PATH> b_dir = toByteArray<MAX_BYTE_PATH>(dir);
	data_dirs->push_back(fi.file_path);
	if (!index_dir->contains(b_dir)) {
		index_dir->insert(b_dir, data_dirs->getLen() - 1);
	}
	else {
		size_t pos = index_dir->search(b_dir).value();
		auto list_ties = data_dirs->getTies(pos);
		data_dirs->toTie(list_ties->front(), data_dirs->getLen() - 1);
	}
	boost::filesystem::path name = path.filename();
	auto byte_name = toByteArray<MAX_BYTE_PATH>(name.wstring());
	data_name->push_back(data->getLen() - 1);
	if (!index_name->contains(byte_name)) {
		index_name->insert(byte_name, data_name->getLen() - 1);
	}
	else {
		size_t pos = index_name->search(byte_name).value();
		auto list_ties = data_name->getTies(pos);
		data_name->toTie(list_ties->front(), data_name->getLen() - 1);
	}
	this->signalInsert(dir);
}

void Storage::insert(const std::wstring& dir_path)
{
	namespace bfs = boost::filesystem;
	if (!bfs::is_directory(dir_path))
		return;
	std::stack <bfs::path> stack;
	stack.push(dir_path);
	while (!stack.empty()) {
		auto p = stack.top();
		stack.pop();
		for (const bfs::directory_entry& entry : bfs::directory_iterator(p)) {
			global::instance().UpdateStatus(SERVICE_RUNNING);
			if (entry.path().filename_is_dot() || entry.path().filename_is_dot_dot())
				continue;
			if (entry.is_directory()) {
				stack.push(entry.path());
			}
			else {
				auto fi = make_info(entry.path().wstring());
				if (fi)
					this->insert(*fi);
			}
		}
	}
}

void Storage::remove(const std::wstring& path)
{
	byte_arr<MAX_BYTE_PATH> key = toByteArray<MAX_BYTE_PATH>(path);
	auto opt = index_path->search(key);
	if (!opt)
		return;
	this->clearName(path);
	auto p_list = data->getTies(opt.value());
	size_t k = 0;
	for (const size_t& index : *p_list) {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		data->remove(index-k);
		this->fixIndex(*index_path, index - k);
		++k;
	}
	index_path->remove(key);
	this->clearDir(path);
}

std::shared_ptr<std::set<std::wstring>> Storage::getAllDir() const
{
	auto result = std::make_shared<std::set<std::wstring>>();
	index_dir->run([&](auto path,auto pos)->void {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		result->insert(fromByteArray<MAX_BYTE_PATH>(path));
		});
	return result;
}

std::shared_ptr<FileInfo> Storage::searchByPath(const std::wstring& path)
{
	auto opt = index_path->search(toByteArray<MAX_BYTE_PATH>(path));
	if (!opt)
		return nullptr;
	return std::make_shared<FileInfo>(data->get(opt.value()));
}

std::shared_ptr<std::list<FileInfo>> Storage::searchByName(const std::wstring& name)
{
	auto result = std::make_shared<std::list<FileInfo>>();
	auto opt = index_name->search(toByteArray<MAX_BYTE_PATH>(name));
	if (!opt)
		return result;
	auto p_list = data_name->getTies(opt.value());
	for (const size_t& index : *p_list) {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		result->push_back(data->get(index));
	}
	return result;
}

std::shared_ptr<std::list<FileInfo>> Storage::getHistory(const std::wstring& path)
{
	auto result = std::make_shared<std::list<FileInfo>>();
	auto opt = index_path->search(toByteArray<MAX_BYTE_PATH>(path));
	if (!opt)
		return nullptr;
	auto p_list_index = data->getTies(opt.value());
	for (const size_t& index : *p_list_index) {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		result->push_front(data->get(index));
	}
	return result;
}

std::shared_ptr<std::list<FileInfo>> Storage::getFilesFromDir(const std::wstring& dir_path)
{
	auto result = std::make_shared<std::list<FileInfo>>();
	auto opt = index_dir->search(toByteArray<MAX_BYTE_PATH>(dir_path));
	if (!opt)
		return result;
	auto indexes = data_dirs->getTies(opt.value());
	for (const size_t& i : *indexes) {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		auto temp = data_dirs->get(i);
		auto opt_pos  = index_path->search(data_dirs->get(i));
		if (!opt_pos)
			return result;
		size_t val = opt_pos.value();
		result->push_front(data->get(opt_pos.value()));
	}
	return result;
}

std::shared_ptr<std::list<FileInfo>> Storage::getAllFiles()
{
	auto result = std::make_shared<std::list<FileInfo>>();
	index_path->run([&](const byte_arr<MAX_BYTE_PATH>& key,size_t value)->void {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		FileInfo fi = data->get(value);
		result->push_front(fi);
		});
	return result;
}

void Storage::clearHistory()
{
	size_t i = 0;
	size_t k = 0;
	data->run([&](FileInfo obj)->void {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		if (obj.change != FileInfo::Changes::EMPTY) {
			data->remove(i-k);
			this->fixIndex(*index_path, i - k);
			++k;
		}
		++i;
		});
}

void Storage::clearHistory(const std::wstring& path)
{
	auto opt = index_dir->search(toByteArray<MAX_BYTE_PATH>(path));
	if (!opt)
		return;
	auto p_list_index = data->getTies(opt.value());
	size_t k = 0;
	for (const size_t& index : *p_list_index) {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		if (data->get(index).change != FileInfo::Changes::EMPTY) {
			data->remove(index - k);
			this->fixIndex(*index_path, index - k);
			++k;
		}
	}
}

void Storage::addToHistory(FileInfo& fi)
{
	if (!index_path->contains(fi.file_path)) {
		return;
	}
	time_t current_time = time(nullptr);
	localtime_s(&fi.change_time, &current_time);
	fi.change_time.tm_year += 1900;
	data->push_back(fi);
	size_t pos = index_path->search(fi.file_path).value();
	auto listTies = data->getTies(pos);
	data->toTie(listTies->front(), data->getLen() - 1);
}

bool Storage::isfileExist(const FileInfo& fi)
{
	return index_path->contains(fi.file_path);
}

Storage::Storage()
{
	boost::filesystem::create_directory(boost::filesystem::current_path() / L"storage");
	boost::filesystem::path p_index_path = boost::filesystem::current_path() / L"storage" / L"index_path.bin";
	this->index_path = std::make_unique<FileMap<byte_arr<MAX_BYTE_PATH>, size_t>>(p_index_path);
	boost::filesystem::path p_data = boost::filesystem::current_path() / L"storage" / L"data.bin";
	this->data = std::make_unique<FileDynamicArray<FileInfo>>(p_data);
	boost::filesystem::path p_data_dirs = boost::filesystem::current_path() / L"storage" / L"data_dirs.bin";
	this->data_dirs = std::make_unique<FileDynamicArray<byte_arr<MAX_BYTE_PATH>>>(p_data_dirs);
	boost::filesystem::path p_index_dir = boost::filesystem::current_path() / L"storage" / L"index_dir.bin";
	this->index_dir = std::make_unique<FileMap<byte_arr<MAX_BYTE_PATH>, size_t>>(p_index_dir);
	boost::filesystem::path p_data_name = boost::filesystem::current_path() / L"storage" / L"data_name.bin";
	this->data_name = std::make_unique<FileDynamicArray<size_t>>(p_data_name);
	boost::filesystem::path p_index_name = boost::filesystem::current_path() / L"storage" / L"index_name.bin";
	this->index_name = std::make_unique<FileMap<byte_arr<MAX_BYTE_PATH>, size_t>>(p_index_name);
}

void Storage::clearDir(const std::wstring& path)
{
	boost::filesystem::path dir_path = boost::filesystem::path(path).parent_path().wstring() + L"\\";
	auto opt_dir = index_dir->search(toByteArray<MAX_BYTE_PATH>(dir_path.wstring()));
	if (!opt_dir)
		throw std::runtime_error("storage is broke");
	std::wstring first_file = fromByteArray(data_dirs->get(opt_dir.value()));
	auto p_list_files = data_dirs->getTies(opt_dir.value());
	if (first_file == path) {
		data_dirs->remove(opt_dir.value());
		p_list_files->pop_back();
		if (!p_list_files->empty())
			return;
		index_dir->remove(toByteArray<MAX_BYTE_PATH>(dir_path.wstring()));
		this->signalRemove(boost::filesystem::path(path).parent_path().wstring() + L"\\");
	}
	for (const size_t& index : *p_list_files) {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		auto byte_path = data_dirs->get(index);
		std::wstring str_path = fromByteArray(byte_path);
		if (str_path == path) {
			data_dirs->remove(index);
			this->fixIndex(*index_path, index);
			break;
		}
	}
}

void Storage::clearName(const std::wstring& path)
{
	std::wstring name = boost::filesystem::path(path).filename().wstring();
	auto opt_name = index_name->search(toByteArray<MAX_BYTE_PATH>(name));
	auto opt_path = index_path->search(toByteArray<MAX_BYTE_PATH>(path));
	if (!opt_name || !opt_path)
		return;
	auto p_list_name = data_name->getTies(opt_name.value());
	size_t delete_index = p_list_name->back();
	for (const size_t& index : *p_list_name) {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		if (data_name->get(index) == opt_path.value()) {
			delete_index = index;
		}
	}
	if (delete_index == p_list_name->back()) {
		data_name->remove(delete_index);
		p_list_name->pop_back();
		if (p_list_name->empty()) {
			index_name->remove(toByteArray<MAX_BYTE_PATH>(name));
			return;
		}
	}
	else {
		data_name->remove(delete_index);
		fixIndex(*index_name, delete_index);
	}
}

void Storage::fixIndex(FileMap<byte_arr<MAX_BYTE_PATH>, size_t>& index,const size_t pos)
{
	index.run([&index,pos](byte_arr<MAX_BYTE_PATH> key, size_t val)->void {
		global::instance().UpdateStatus(SERVICE_RUNNING);
		if (val > pos) {
			index.changeValue(key, val - 1);
		}
		});
}

#ifdef _DEBUG
void Storage::print() const
{
	std::wcout << L"---storage---" << std::endl;
	data->run([](FileInfo el)->void {
		std::wcout << fromByteArray(el.file_path) << std::endl;
		});
	std::wcout << L"---index_path---" << std::endl;
	index_path->run([](auto el, size_t pos)->void {
		std::wcout << fromByteArray(el) << " " << pos << std::endl;
		});
	std::wcout << L"---data_dirs---" << std::endl;
	data_dirs->run([](auto el)->void {
		std::wcout << fromByteArray(el) << std::endl;
		});
	std::wcout << L"---index_dir---" << std::endl;
	index_dir->run([](auto el, size_t pos)->void {
		std::wcout << fromByteArray(el) << " " << pos << std::endl;
		});
	std::wcout << L"---data_name---" << std::endl;
	data_name->run([](auto el)->void {
		std::wcout << el << std::endl;
		});
	std::wcout << L"---index_name---" << std::endl;
	index_name->run([](auto key,auto val)->void {
		std::wcout << fromByteArray(key) << " " << val << std::endl;
		});
}
#endif
