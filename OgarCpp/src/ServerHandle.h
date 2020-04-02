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
#include "protocols/ProtocolStore.h"
#include "gamemodes/GamemodeList.h"
#include "gamemodes/Gamemode.h"

class Gamemode;
class GamemodeList;

using namespace libconfig;
using namespace std::chrono;

struct RuntimeSettings {
	double worldEatMult;
	double worldEatOverlapDiv;
	int worldSafeSpawnTries;
	double worldSafeSpawnFromEjectedChance;
	int pelletMinSize;
	int pelletMaxSize;
	int pelletGrowTicks;
	int pelletCount;
	int virusMinCount;
	int virusMaxCount;
	double virusSize;
	int virusFeedTimes;
	bool virusPushing;
	double virusSplitBoost;
	double virusPushBoost;
	bool virusMonotonePops;
	double ejectedSize;
	double ejectingLoss;
	double ejectDispersion;
	double ejectedCellBoost;
	double mothercellSize;
	int mothercellCount;
	double mothercellPassiveSpawnChance;
	double mothercellActiveSpawnSpeed;
	double mothercellPelletBoost;
	int mothercellMaxPellets;
	double mothercellMaxSize;
	double playerRoamSpeed;
	double playerRoamViewScale;
	double playerViewScaleMult;
	double playerMinViewScale;
	int playerMaxNameLength;
	bool playerAllowSkinInName;
	double playerMinSize = 32.0;
	double playerSpawnSize = 32.0;
	double playerMaxSize = 1500.0;
	double playerMinSplitSize = 60.0;
	double playerMinEjectSize = 60.0;
	int playerSplitCap = 255;
	int playerEjectDelay = 2;
	int playerMaxCells = 16;

	double playerMoveMult = 1;
	double playerSplitSizeDiv = 1.414213562373095;
	double playerSplitDistance = 40;
	double playerSplitBoost = 780;
	double playerNoCollideDelay = 13;
	int playerNoMergeDelay = 15;
	bool playerMergeNewVersion = false;
	int playerMergeTime = 30;
	double playerMergeTimeIncrease = 0.02;
	double playerDecayMult = 0.001;
};

class ServerHandle {
public:
	Setting* settings;

	ProtocolStore* protocols;
	GamemodeList*  gamemodes;

	Gamemode* gamemode;
	RuntimeSettings runtime;

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