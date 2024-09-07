#pragma once
#include <boost/filesystem/path.hpp>
#include <memory>
#include <string>
#include <time.h>
#include <iomanip>
#include <codecvt>
#include <sstream>
#include <Windows.h>
#define MAX_BYTE_PATH 520

template<unsigned short size>
struct byte_arr{ 
	char data[size];
	uint16_t len = 0;
	friend bool operator==(const byte_arr& b1,const byte_arr& b2) {
		if (b1.len != b2.len)
			return false;
		for (size_t i = 0; i < b1.len; ++i) {
			if (b1.data[i] != b2.data[i]) {
				return false;
			}
		}
		return true;
	}
	friend bool operator!=(const byte_arr& b1, const byte_arr& b2) {
		return !(b1 == b2);
	}
};

struct TM : public tm
{
	friend bool operator==(const TM& first, const TM& second);
	friend bool operator!=(const TM& first, const TM& second);
	friend bool operator<(const TM& first, const TM& second);
	friend bool operator>(const TM& first, const TM& second);
	friend bool operator>=(const TM& first, const TM& second);
	friend bool operator<=(const TM& first, const TM& second);
	operator tm();
	static TM convert(const tm& src);
};

class FileInfo {
public:
	enum class Changes {
		EMPTY = 0,
		ADDED = 1,
		REMOVED = 2,
		MODIFIED = 3,
		RENAMED_OLD = 4,
		RENAMED_NEW = 5,
	};
public:
	byte_arr<MAX_BYTE_PATH> file_path;
	SYSTEMTIME creation_time = {};
	SYSTEMTIME last_access_time = {};
	SYSTEMTIME last_write_time = {};
	size_t file_size = 0;
	unsigned short num_of_links = 0;
	unsigned short attribute = 0;
	Changes change = Changes::EMPTY;
	tm change_time{0};
public:
	FileInfo();
	FileInfo(const std::wstring& path, const BY_HANDLE_FILE_INFORMATION& file_info, Changes changes);
	FileInfo(const FileInfo& fi);
	FileInfo& operator=(const FileInfo& fi);
	static std::wstring changeToStr(const Changes& change);
};

std::shared_ptr<FileInfo> make_info(const std::wstring& path_to_file);

template<unsigned short size>
byte_arr<size> toByteArray(const std::wstring& str) {
	byte_arr<size> res;
	res.len = str.size() * sizeof(wchar_t);
	for (size_t i = 0; i < res.len; ++i) {
		res.data[i] = ((char*)str.c_str())[i];
	}
	return res;
}

template<unsigned short size>
std::wstring fromByteArray(const byte_arr<size>& arr) {
	std::wstring result;
	for (size_t i = 0; i < arr.len; i += 2) {
		wchar_t sumb;
		((char*)&sumb)[0] = ((char*)&arr.data)[i];
		((char*)&sumb)[1] = ((char*)&arr.data)[i + 1];
		result += sumb;
	}
	return result;
}

tm strToTime(std::wstring str);
tm strToTime(std::string str);
std::wstring timeToStr(tm time);
tm systemTimeToTm(const SYSTEMTIME& sysTime);
std::string toUTF8(const std::wstring& src);
std::wstring fromUTF8(const std::string& src);