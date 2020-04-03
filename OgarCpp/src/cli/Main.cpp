#define _HAS_STD_BYTE 0
#define _HAS_STD_BOOLEAN 0

#include <iostream>

#include "../ServerHandle.h"
#include "../Settings.h"

#include "../protocols/ModernProtocol.h"
#include "../protocols/Protocol6.h"
#include "../gamemodes/FFA.h"

bool exitCLI = false;
Setting* settings = loadConfig();

void registerStuff(ServerHandle* handle) {
	auto ffa = new FFA(handle);
	handle->gamemodes->registerGamemode(ffa);
	handle->gamemodes->setGamemode("FFA");
	Protocol* ptc = new ModernProtocol(nullptr);
	handle->protocols->registerProtocol(ptc);
	ptc = new Protocol6(nullptr);
	handle->protocols->registerProtocol(ptc);
}

int main() {
	
	auto handle = new ServerHandle(settings);
	registerStuff(handle);

	Command<ServerHandle*> startCommand("start", "start the handle", "", 
		[handle](ServerHandle* handle, auto context, vector<string>& args) {
		if (!handle->start()) Logger::info("Handle already running");
	});
	handle->commands.registerCommand(startCommand);

	Command<ServerHandle*> stopCommand("stop", "stop the handle", "",
		[handle](ServerHandle* handle, auto context, vector<string>& args) {
		if (!handle->stop()) Logger::info("Handle not running");
	});
	handle->commands.registerCommand(stopCommand);

	Command<ServerHandle*> exitCommand("exit", "stop the handle and close the command stream", "",
		[handle](ServerHandle* handle, auto context, vector<string>& args) {
		handle->stop();
		exitCLI = true;
	});
	handle->commands.registerCommand(exitCommand);

	Command<ServerHandle*> reloadCommand("reload", "reload the settings from local game.cfg", "",
		[handle](ServerHandle* handle, auto context, vector<string>& args) {
		settings = loadConfig();
		handle->setSettings(settings);
	});
	handle->commands.registerCommand(reloadCommand);

	Command<ServerHandle*> saveCommand("save", "save the current settings to game.cfg", "",
		[handle](ServerHandle* handle, auto context, vector<string>& args) {
		saveConfig();
	});
	handle->commands.registerCommand(saveCommand);

	handle->start();
	
	string input;
	while (!exitCLI) {
		std::getline(std::cin, input);
		input = trim(input);
		if (!input.length()) continue;
		if (!handle->commands.execute(nullptr, input))
			Logger::info("Unknown command");
	}

	delete handle;

	return EXIT_SUCCESS;
}