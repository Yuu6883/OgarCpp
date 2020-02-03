#pragma once

#include <string>
#include <iostream>
using namespace std;

#define DEBUG   0
#define VERBOSE 1
#define INFO    2
#define WARN    3
#define ERROR   4
#define NOTHING 5

static int logLevel = DEBUG;
static ostream* out = &cout;
static bool color = true;

void setLogLevel(int level) {
	logLevel = level;
}

void debug(string_view string) {
	if (!out || logLevel > DEBUG) return;
	if (color) {
		*out << "[\033[92mdebug\033[0m] " << string << endl;
	}
	else {
		*out << "[debug]" << string << endl;
	}
}

void verbose(string_view string) {
	if (!out || logLevel > VERBOSE) return;
	if (color) {
		*out << "[\033[95mverbose\033[0m] " << string << endl;
	}
	else {
		*out << "[verbose]" << string << endl;
	}
}

void info(string_view string) {
	if (!out || logLevel > INFO) return;
	if (color) {
		*out << "[\033[96minfo\033[0m] " << string << endl;
	}
	else {
		*out << "[info]" << string << endl;
	}
}

void warn(string_view string) {
	if (!out || logLevel > WARN) return;
	if (color) {
		*out << "[\033[93mwarn\033[0m] " << string << endl;
	}
	else {
		*out << "[warn]" << string << endl;
	}
}

void error(string_view string) {
	if (!out || logLevel > ERROR) return;
	if (color) {
		*out << "[\033[91merror\033[0m] " << string << endl;
	} else {
		*out << "[error]" << string << endl;
	}
}