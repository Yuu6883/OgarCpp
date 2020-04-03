#pragma once

#include <random>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <memory>
#include <string>
#include <stdexcept>

#define randomZeroToOne ((double) rand() / (RAND_MAX))

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

using std::string;

static const unsigned char OGAR_VERSION[3] = { 1, 3, 6 };
static const string OGAR_VERSION_STRING = "1.3.6";

inline unsigned int randomColor() {
	switch ((unsigned char)(randomZeroToOne * 6)) {
        case 0: return ((unsigned char)(randomZeroToOne * 0x100) << 16) | (0xFF << 8) | 0x10;
        case 1: return ((unsigned char)(randomZeroToOne * 0x100) << 16) | (0x10 << 8) | 0xFF;
        case 2: return (0xFF << 16) | ((unsigned char)(randomZeroToOne * 0x100) << 8) | 0x10;
        case 3: return (0x10 << 16) | ((unsigned char)(randomZeroToOne * 0x100) << 8) | 0xFF;
        case 4: return (0x10 << 16) | (0xFF << 8) | (unsigned char)(randomZeroToOne * 0x100);
        case 5: return (0xFF << 16) | (0x10 << 8) | (unsigned char)(randomZeroToOne * 0x100);
	}
    return 0;
}

// trim from start (in place)
inline string ltrim(string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    return s;
}

// trim from end (in place)
inline string rtrim(string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    return s;
}

// trim from both ends (in place)
inline string trim(string s) {
    return rtrim(ltrim(s));
}
