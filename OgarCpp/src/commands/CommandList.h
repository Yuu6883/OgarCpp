#pragma once

#include <vector>
#include <algorithm>
#include <map>
#include <list>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iterator>
#include <functional>
#include <mutex>

using std::string;
using std::vector;
using std::function;

class ServerHandle;

static void toLowerCase(string* str) {
	std::transform(str->begin(), str->end(), str->begin(), [](unsigned char c) { return tolower(c); });
};

template<class T>
class Command {

public:
	typedef function<void(ServerHandle*, T, vector<string>&)> CommandExecutor;
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
	std::map<string, Command<T>> commands;
	std::mutex lock;
	std::list<std::pair<T, string>> pendingCommand;
public:
	CommandList(ServerHandle* handle) : handle(handle) {};

	void registerCommand(Command<T>& command) {
		if (commands.find(command.name) != commands.cend()) {
			Logger::error(string("Command \"") + command.name + string("\" is already registered."));
		} else {
			commands.insert(std::make_pair(command.name, command));
		}
	};

	void execute(T context, string input) {
		std::lock_guard l(lock);
		pendingCommand.push_back(std::make_pair(context, input));
	};

	void process() {
		std::lock_guard l(lock);
		for (auto [context, input] : pendingCommand) {
			std::stringstream ss(input);
			std::istream_iterator<string> begin(ss);
			std::istream_iterator<string> end;
			std::vector<string> tokens(begin, end);

			if (tokens.empty()) continue;
			string cmd = tokens[0];
			tokens.erase(tokens.begin());

			if (commands.find(cmd) != commands.cend()) {
				if (commands.at(cmd).executor) {
					commands.at(cmd).executor(handle, context, tokens);
					continue;
				}
			}
			Logger::warn(string("Unknown command: ") + cmd);
		}
		pendingCommand.clear();
	}
};