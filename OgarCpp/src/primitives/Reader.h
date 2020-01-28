#pragma once

#include <string>
#include <exception>
using namespace std;

class Reader {
	const char* origin;
	char* charPtr;
	size_t size;

public:
	Reader(const char* origin) : 
		origin(origin), charPtr((char*) origin), size(sizeof(origin)) {};

	void reset() {
		charPtr = (char*) origin;
	}

	int readUInt8() {
		checkBound(1);
		return (unsigned char) *charPtr++;
	}

	int readInt8() {
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

	float readFloat32() {
		checkBound(4);
		auto value = *((float*) charPtr);
		charPtr += 4;
		return value;
	}

	double readFloat64() {
		checkBound(8);
		auto value = *((double*) charPtr);
		charPtr += 8;
		return value;
	}

	void skip(long count) {
		charPtr += count;
	}

	string readStringUTF8() {
		return string(charPtr);
	}

	void checkBound(int limit) {
		if (charPtr - origin > size - limit) {
			throw runtime_error("ReaderOutOfBoundException: [size=" + to_string(size) + "]");
		}
	}

	int readColor() {
		// TODO
		return 0;
	}
};