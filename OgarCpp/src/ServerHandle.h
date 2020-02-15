#pragma once

#include <libconfig.h++>
#include <chrono>
#include "commands/CommandList.h"
#include "misc/Ticker.h"
#include "misc/Stopwatch.h"

using namespace libconfig;
using namespace std::chrono;

class Player;
class Router;

class ServerHandle {

public:
	Setting* settings;
	CommandList<ServerHandle*> commands;
	CommandList<ServerHandle*> chatCommands;

	bool running = false;
	unsigned long tick = -1;
	int tickDelay = -1;
	int stepMult = -1;

	float averageTickTime = 0;

	Ticker ticker;
	Stopwatch stopwatch;
	
	time_point<system_clock> startTime = system_clock::now();

	ServerHandle(Setting* settings);

	void setSettings(Setting* settings);
	
	int getSettingInt(const char* key);
	bool getSettingBool(const char* key);
	double getSettingDouble(const char* key);
	std::string getSettingString(const char* key);

	void onTick();
	bool start();
	bool stop();
	Player* createPlayer(Router*);
	bool removePlayer(int);
};

#include "sockets/Router.h"
#include "sockets/Listener.h"