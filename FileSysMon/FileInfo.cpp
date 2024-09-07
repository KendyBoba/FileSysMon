#include "FileInfo.h"

FileInfo::FileInfo()
{
}

FileInfo::FileInfo(const std::wstring& path, const BY_HANDLE_FILE_INFORMATION& file_info, Changes changes)
{
	this->file_path = toByteArray<MAX_BYTE_PATH>(path);
	FileTimeToSystemTime(&file_info.ftCreationTime, &this->creation_time);
	FileTimeToSystemTime(&file_info.ftLastAccessTime, &this->last_access_time);
	FileTimeToSystemTime(&file_info.ftLastWriteTime, &this->last_write_time);
	this->file_size = MAKELONG(file_info.nFileSizeLow, file_info.nFileSizeHigh);
	this->change = changes;
	this->num_of_links = file_info.nNumberOfLinks;
}

FileInfo::FileInfo(const FileInfo& fi)
{
	ZeroMemory(this->file_path.data, MAX_BYTE_PATH);
	for (size_t i = 0; i < fi.file_path.len; ++i) {
		this->file_path.data[i] = fi.file_path.data[i];
	}
	this->file_path.len = fi.file_path.len;
	this->file_size = fi.file_size;
	this->num_of_links = fi.num_of_links;
	this->attribute = fi.attribute;
	this->creation_time = fi.creation_time;
	this->last_access_time = fi.last_access_time;
	this->last_write_time = fi.last_write_time;
	this->change = fi.change;
	this->change_time = fi.change_time;
}

FileInfo& FileInfo::operator=(const FileInfo& fi)
{
	if (this == &fi)
		return *this;
	ZeroMemory(this->file_path.data, MAX_BYTE_PATH);
	for (size_t i = 0; i < fi.file_path.len; ++i) {
		this->file_path.data[i] = fi.file_path.data[i];
	}
	this->file_path.len = fi.file_path.len;
	this->file_size = fi.file_size;
	this->num_of_links = fi.num_of_links;
	this->attribute = fi.attribute;
	this->creation_time = fi.creation_time;
	this->last_access_time = fi.last_access_time;
	this->last_write_time = fi.last_write_time;
	this->change = fi.change;
	this->change_time = fi.change_time;
	return *this;
}

std::wstring FileInfo::changeToStr(const Changes& change)
{
	switch (change)
	{
	case Changes::ADDED: return L"ADD"; break;
	case Changes::REMOVED: return L"REMOVE"; break;
	case Changes::MODIFIED: return L"MODIFY"; break;
	case Changes::RENAMED_OLD:
	case Changes::RENAMED_NEW: return L"RENAME"; break;
	}
	return L"NONE";
}

bool operator==(const TM& first, const TM& second)
{
	return (first.tm_year == second.tm_year && first.tm_mon == second.tm_mon
		&& first.tm_mday == second.tm_mday && first.tm_hour == second.tm_hour
		&& first.tm_min == second.tm_min && first.tm_sec == second.tm_sec);
}

bool operator!=(const TM& first, const TM& second)
{
	return !(first == second);
}

bool operator<(const TM& first, const TM& second)
{
	int* arr_first = (int*)&first;
	int* arr_second = (int*)&second;
	for (unsigned short i = 4; i >= 0; --i) {
		if (arr_first[i] < arr_second[i])
			return true;
		if (arr_first[i] > arr_second[i])
			return false;
	}
	return false;
}

bool operator>(const TM& first, const TM& second)
{
	return first != second && !(first < second);
}

bool operator>=(const TM& first, const TM& second)
{
	return first > second || first == second;
}

bool operator<=(const TM& first, const TM& second)
{
	return first < second || first == second;
}

std::shared_ptr<FileInfo> make_info(const std::wstring& path_to_file)
{
	HANDLE file = CreateFile(
		path_to_file.c_str(),
		GENERIC_ALL,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (file == INVALID_HANDLE_VALUE)
		return nullptr;
	BY_HANDLE_FILE_INFORMATION fi;
	ZeroMemory(&fi, sizeof(fi));
	if (!GetFileInformationByHandle(file, &fi)) {
		CloseHandle(file);
		return nullptr;
	}
	CloseHandle(file);
	return std::make_shared<FileInfo>(path_to_file, fi, FileInfo::Changes::EMPTY);
}

tm strToTime(std::wstring str)
{
	tm result;
	std::wstringstream ss(str);
	ss >> std::get_time(&result, L"%d.%m.%Y %H:%M:%S");
	result.tm_year += 1900;
	return result;
}

tm strToTime(std::string str)
{
	tm result;
	std::stringstream ss(str);
	ss >> std::get_time(&result, "%d.%m.%Y %H:%M:%S");
	result.tm_year += 1900;
	return result;
}

std::wstring timeToStr(tm time) {
	time.tm_year -= 1900;
	std::stringstream ss;
	ss << std::put_time(&time, "%d.%m.%Y %H:%M:%S");
	std::string res = ss.str();
	return std::wstring(res.begin(), res.end());
}

tm systemTimeToTm(const SYSTEMTIME& sysTime) {
	tm res;
	FILETIME ft{ 0 };
	SystemTimeToFileTime(&sysTime, &ft);
	time_t time = (*((time_t*)&ft) / 10000000.0 - 11644473600.0);
	localtime_s(&res, &time);
	res.tm_year += 1900;
	return res;
}

std::string toUTF8(const std::wstring& src)
{
	std::wstring_convert< std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(src);
}

std::wstring fromUTF8(const std::string& src) {
	std::wstring_convert< std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(src);
}

TM::operator tm()
{
	tm res;
	res.tm_year = this->tm_year;
	res.tm_mon = this->tm_mon;
	res.tm_mday = this->tm_mday;
	res.tm_hour = this->tm_hour;
	res.tm_min = this->tm_min;
	res.tm_sec = this->tm_sec;
	return res;
}

TM TM::convert(const tm& src)
{
	TM res;
	res.tm_year = src.tm_year;
	res.tm_mon = src.tm_mon;
	res.tm_mday = src.tm_mday;
	res.tm_hour = src.tm_hour;
	res.tm_min = src.tm_min;
	res.tm_sec = src.tm_sec;
	return res;
}
