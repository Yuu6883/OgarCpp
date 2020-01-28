#pragma once

#include <vector>
#include <algorithm>
#include <map>
#include <string.h>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "../ServerHandle.h"

using namespace std;
using namespace boost::algorithm;

class ServerHandle;

static void toLowerCase(string* str) {
	transform(str->begin(), str->end(), str->begin(), [](unsigned char c) { return tolower(c); });
};

template<class T>
class Command {

public:
	typedef void (*CommandExecutor)(ServerHandle*, T, vector<string>&);
	string name;
	string description;
	string args;
	CommandExecutor executor;

	Command() : name(""), description(""), args(""), executor(nullptr) {};
	Command(string name, string description, string args = "", CommandExecutor executor = nullptr) :
		description(description), args(args), executor(executor) {
		toLowerCase(&name);
		this->name = name;
	}
};

template<class T>
ostream& operator<<(ostream& stream, const Command<T>& cmd) {
	stream << cmd.name;
	if (cmd.args.length()) {
		stream << " " << cmd.args;
	}
	return stream << " - " << cmd.description;
};

template<class T>
class CommandList {

private:
	ServerHandle* handle;
	map<string, Command<T>> commands;

public:
	CommandList(ServerHandle* handle) : handle(handle) {};

	void registerCommand(Command<T>& command) {
		if (commands.contains(command.name)) {
			cerr << "Command \"" << command.name << "\" is already registered." << endl;
		} else {
			commands.insert(make_pair(command.name, command));
		}
	};

	bool execute(T context, string input) {
		vector<string> tokens;
		split(tokens, input, is_space());

		if (tokens.empty()) return false;
		string cmd = tokens[0];
		tokens.erase(tokens.begin());

		if (commands.contains(cmd)) {
			if (commands.at(cmd).executor) {
				commands.at(cmd).executor(handle, context, tokens);
			}
			return true;
		} else return false;
	};
};