#pragma once

#include <libconfig.h++>
#include <chrono>
#include <map>
#include "primitives/Logger.h"
#include "gamemodes/Gamemode.h"
#include "commands/CommandList.h"
#include "misc/Ticker.h"
#include "misc/Stopwatch.h"
#include "sockets/Router.h"
#include "sockets/Listener.h"
#include "worlds/World.h"

using namespace libconfig;
using namespace std::chrono;

class ServerHandle {

public:

	Setting* settings;
	Gamemode* gamemode = nullptr;

	CommandList<ServerHandle*> commands     = CommandList<ServerHandle*>(this);
	CommandList<ServerHandle*> chatCommands = CommandList<ServerHandle*>(this);

	bool running = false;
	unsigned long tick = -1;
	int tickDelay = -1;
	int stepMult = -1;

	float averageTickTime = 0;

	Ticker ticker;
	Stopwatch stopwatch;
	
	time_point<system_clock> startTime = system_clock::now();
	Listener listener = Listener(this);

	std::map<unsigned int, World*>  worlds;
	std::map<unsigned int, Player*> players;

	ServerHandle(Setting* settings);

	void setSettings(Setting* settings);
	
	int getSettingInt(const char* key);
	bool getSettingBool(const char* key);
	double getSettingDouble(const char* key);
	std::string getSettingString(const char* key);

	void onTick();
	bool start();
	bool stop();
	Player* createPlayer(Router* router);
	bool removePlayer(unsigned int id);
	World* createWorld();
	bool removeWorld(unsigned int id);
};