#pragma once

#include <vector>
#include <algorithm>
#include <map>
#include <string.h>
#include <iostream>
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;

class ServerHandle;

static void toLowerCase(std::string* str) {
	std::transform(str->begin(), str->end(), str->begin(), [](unsigned char c) { return tolower(c); });
};

template<class T>
class Command {

public:
	typedef void (*CommandExecutor)(ServerHandle*, T, std::vector<std::string>&);
	std::string name;
	std::string description;
	std::string args;
	CommandExecutor executor;

	Command() : name(""), description(""), args(""), executor(nullptr) {};
	Command(std::string name, std::string description, std::string args = "", CommandExecutor executor = nullptr) :
		description(description), args(args), executor(executor) {
		toLowerCase(&name);
		this->name = name;
	}
};

template<class T>
std::ostream& operator<<(std::ostream& stream, const Command<T>& cmd) {
	stream << cmd.name;
	if (cmd.args.length()) {
		stream << " " << cmd.args;
	}
	return stream << " - " << cmd.description;
};

template<class T>
class CommandList {
	friend ServerHandle;
private:
	ServerHandle* handle;
	std::map<std::string, Command<T>> commands;

public:
	CommandList(ServerHandle* handle = nullptr) : handle(handle) {};

	void registerCommand(Command<T>& command) {
		if (commands.contains(command.name)) {
			error(std::string("Command \"") + command.name + std::string("\" is already registered."));
		} else {
			commands.insert(std::make_pair(command.name, command));
		}
	};

	bool execute(T context, std::string input) {
		std::vector<std::string> tokens;
		split(tokens, input, is_space());

		if (tokens.empty()) return false;
		std::string cmd = tokens[0];
		tokens.erase(tokens.begin());

		if (commands.contains(cmd)) {
			if (commands.at(cmd).executor) {
				commands.at(cmd).executor(handle, context, tokens);
			}
			return true;
		} else return false;
	};
};