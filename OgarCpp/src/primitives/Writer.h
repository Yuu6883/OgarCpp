#pragma once

#include <stdlib.h>
#include <string>
#include <thread>
#include <map>

using namespace std;

constexpr auto POOL_SIZE = 1024 * 1024;
map<thread::id, char*> BufferMap;

class Writer {
	char* pool;
	char* ptr;
public:
	Writer() {
		auto thread_id = this_thread::get_id();
		if (BufferMap.contains(thread_id)) {
			pool = BufferMap.at(thread_id);
		} else {
			pool = (char*) malloc(POOL_SIZE);
			BufferMap.insert(make_pair(thread_id, pool));
		}
		ptr = pool;
	}

	~Writer() {
		delete pool;
	}

	int offset() {
		return ptr - pool;
	}

	void writeUInt8(const unsigned char& value) {
		*ptr = value;
		ptr++;
	}

	void writeInt8(const char& value) {
		*ptr = value;
		ptr++;
	}

	void writeUInt16(const unsigned short& value) {
		*((unsigned short *) ptr) = value;
		ptr += 2;
	}

	void writeInt16(const short& value) {
		*((short*) ptr) = value;
		ptr += 2;
	}

	void writeUInt32(const unsigned int& value) {
		*((unsigned int*) ptr) = value;
		ptr += 4;
	}

	void writeInt32(const int& value) {
		*((int*) ptr) = value;
		ptr += 4;
	}

	void writeUInt64(const unsigned long& value) {
		*((unsigned long*)ptr) = value;
		ptr += 8;
	}

	void writeInt64(const long& value) {
		*((long*)ptr) = value;
		ptr += 8;
	}

	void writeFloat32(const float& value) {
		*((float*) ptr) = value;
		ptr += 4;
	}

	void writeFloat64(const double& value) {
		*((double*) ptr) = value;
		ptr += 8;
	}

	void writeStringUTF8(const char* string) {
		memcpy(ptr, string, strlen(string) + 1);
		auto len = strlen(string);
		ptr += len;
		*ptr = 0;
		ptr++;
	}

	string_view finalize() {
		int offset = this->offset();
		char* result = (char *) malloc(offset);
		memcpy(result, pool, offset);
		return string_view(result, offset);
	}
};