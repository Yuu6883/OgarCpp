#pragma once

#include <string>
#include <exception>
#include <stdexcept>

class Reader {

	const char* origin;
	char* charPtr;
	int size;

	void checkBound(int limit) {
		if (offset() + limit > size) {
			throw std::runtime_error("ReaderOutOfBoundException: [size=" + std::to_string(size) + "]");
		}
	}

public:
	Reader(std::string_view view) : 
		origin(view.data()), charPtr((char*) view.data()), size(view.size()) {};

	void reset() {
		charPtr = (char*) origin;
	}

	int length() {
		return size;
	}

	unsigned char readUInt8() {
		checkBound(1);
		return (unsigned char) *charPtr++;
	}

	char readInt8() {
		checkBound(1);
		return (char) *charPtr++;
	}

	unsigned short readUInt16() {
		checkBound(2);
		auto value = *((unsigned short*) charPtr);
		charPtr += 2;
		return value;
	}

	short readInt16() {
		checkBound(2);
		auto value = *((short*) charPtr);
		charPtr += 2;
		return value;
	}

	unsigned int readUInt32() {
		checkBound(4);
		auto value = *((unsigned int*) charPtr);
		charPtr += 4;
		return value;
	}

	int readInt32() {
		checkBound(4);
		auto value = *((int*) charPtr);
		charPtr += 4;
		return value;
	}

	unsigned long readUInt64() {
		checkBound(8);
		auto value = *((unsigned long*) charPtr);
		charPtr += 8;
		return value;
	}

	long readInt64() {
		checkBound(8);
		auto value = *((long*) charPtr);
		charPtr += 8;
		return value;
	}

	float readFloat32() {
		checkBound(4);
		auto value = *((float*) charPtr);
		charPtr += 4;
		return value;
	}

	float readFloat64() {
		checkBound(8);
		auto value = *((float*) charPtr);
		charPtr += 8;
		return value;
	}

	void skip(long count) {
		charPtr += count;
	}

	void back(long count) {
		charPtr -= count;
	}

	int offset() {
		return charPtr - origin;
	}

	std::string readStringUTF8() {
		std::string result(charPtr);
		skip(strlen(charPtr) + 1);
		return result;
	}
};