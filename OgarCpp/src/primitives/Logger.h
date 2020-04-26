#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <mutex>

#define L_DEBUG   0
#define L_VERBOSE 1
#define L_INFO    2
#define L_WARN    3
#define L_ERROR   4
#define L_NOTHING 5

static int logLevel = L_DEBUG;
static std::ostream* out = &std::cout;
static bool color = true;
static std::mutex lock;

using std::string;
using std::string_view;

class Logger {
public:
	static void setLogLevel(int level) {
		logLevel = level;
	}

	static void debug(std::string_view string) {
		if (!out || logLevel > L_DEBUG) return;
        std::lock_guard l(lock);
		if (color) {
			*out << "[\033[92mdebug\033[0m] " << string << std::endl;
		}
		else {
			*out << "[debug]" << string << std::endl;
		}
		out->flush();
	}

	static void verbose(std::string_view string) {
		if (!out || logLevel > L_VERBOSE) return;
        std::lock_guard l(lock);
		if (color) {
			*out << "[\033[95mverbose\033[0m] " << string << std::endl;
		}
		else {
			*out << "[verbose]" << string << std::endl;
		}
		out->flush();
	}

	static void info(std::string_view string) {
		if (!out || logLevel > L_INFO) return;
        std::lock_guard l(lock);
		if (color) {
			*out << "[\033[96minfo\033[0m] " << string << std::endl;
		}
		else {
			*out << "[info]" << string << std::endl;
		}
		out->flush();
	}

	static void warn(std::string_view string) {
		if (!out || logLevel > L_WARN) return;
        std::lock_guard l(lock);
		if (color) {
			*out << "[\033[93mwarn\033[0m] " << string << std::endl;
		}
		else {
			*out << "[warn]" << string << std::endl;
		}
		out->flush();
	}

	static void error(std::string_view string) {
		if (!out || logLevel > L_ERROR) return;
        std::lock_guard l(lock);
		if (color) {
			*out << "[\033[91merror\033[0m] " << string << std::endl;
		}
		else {
			*out << "[error]" << string << std::endl;
		}
		out->flush();
	}
};
