#pragma once

#include <stdlib.h>
#include <string>
#include <thread>
#include <map>

using std::string_view;
constexpr auto POOL_SIZE = 2 * 1024 * 1024;
static const thread_local char* pool = (char*) malloc(POOL_SIZE);

class Writer {
	char* ptr;
public:

	Writer() {
		ptr = (char*) pool;
	}

	const char* getPool() { return pool; };

	~Writer() {}

	void reset() {
		ptr = (char*)pool;
	}

	unsigned int offset() {
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

	void writeColor(const unsigned int& value) {
		*((unsigned int*) ptr) = ((value & 0xFF) << 16) | (((value >> 8) & 0xFF) << 8) | (value >> 16);
		ptr += 3;
	}

	void writeBuffer(const string_view& view) {
		memcpy(ptr, view.data(), view.size());
		ptr += view.size();
	}

	string_view finalize() {
		auto offset = this->offset();
		if (offset > POOL_SIZE * 0.9f) {
			printf("WARNING: OFFSET = %i", offset);
		}
		return string_view(pool, offset);
	}
};