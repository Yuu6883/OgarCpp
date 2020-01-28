#pragma once

#include <libconfig.h++>
#include <chrono>
#include "commands/CommandList.h"

using namespace libconfig;
using namespace std::chrono;

class ServerHandle {

public:
	Setting* settings;
	CommandList<ServerHandle*>* commands = nullptr;
	CommandList<ServerHandle*>* chatCommands = nullptr;

	bool running = false;
	int tick = -1;
	int tickDelay = -1;
	int tickMult = -1;
	
	time_point<system_clock> startTime = system_clock::now();
	ServerHandle(Setting* settings);
	void setSettings(Setting* settings);
};