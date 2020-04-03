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
#include "worlds/MatchMaker.h"
#include "protocols/ProtocolStore.h"
#include "gamemodes/GamemodeList.h"
#include "gamemodes/Gamemode.h"

class Gamemode;
class GamemodeList;

using namespace libconfig;
using namespace std::chrono;

struct RuntimeSettings {
	string serverName;
	bool chatEnabled;
	int worldMaxPlayers;
	int worldMaxCount;
	int listenerMaxConnections;
	int chatCooldown;
	int matchmakerBulkSize;
	bool minionEnableQBasedControl;
	bool matchmakerNeedsQueuing;
	float minionSpawnSize;
	float worldEatMult;
	float worldEatOverlapDiv;
	int worldSafeSpawnTries;
	float worldSafeSpawnFromEjectedChance;
	int worldPlayerDisposeDelay;
	int pelletMinSize;
	int pelletMaxSize;
	int pelletGrowTicks;
	int pelletCount;
	int virusMinCount;
	int virusMaxCount;
	float virusSize;
	int virusFeedTimes;
	bool virusPushing;
	float virusSplitBoost;
	float virusPushBoost;
	bool virusMonotonePops;
	float ejectedSize;
	float ejectingLoss;
	float ejectDispersion;
	float ejectedCellBoost;
	float mothercellSize;
	int mothercellCount;
	float mothercellPassiveSpawnChance;
	float mothercellActiveSpawnSpeed;
	float mothercellPelletBoost;
	int mothercellMaxPellets;
	float mothercellMaxSize;
	float playerRoamSpeed;
	float playerRoamViewScale;
	float playerViewScaleMult;
	float playerMinViewScale;
	int playerMaxNameLength;
	bool playerAllowSkinInName;
	float playerMinSize = 32.0;
	float playerSpawnSize = 32.0;
	float playerMaxSize = 1500.0;
	float playerMinSplitSize = 60.0;
	float playerMinEjectSize = 60.0;
	int playerSplitCap = 255;
	int playerEjectDelay = 2;
	int playerMaxCells = 16;

	float playerMoveMult = 1;
	float playerSplitSizeDiv = 1.414213562373095;
	float playerSplitDistance = 40;
	float playerSplitBoost = 780;
	float playerNoCollideDelay = 13;
	int playerNoMergeDelay = 15;
	bool playerMergeNewVersion = false;
	int playerMergeTime = 30;
	float playerMergeTimeIncrease = 0.02;
	float playerDecayMult = 0.001;
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

	float averageTickTime = 0.0;

	Ticker ticker;
	Stopwatch stopwatch;
	
	time_point<system_clock> startTime = system_clock::now();
	Listener listener = Listener(this);
	MatchMaker matchmaker = MatchMaker(this);

	std::map<unsigned int, World*>  worlds;
	std::map<unsigned int, Player*> players;

	ServerHandle(Setting* settings);
	~ServerHandle();

	void setSettings(Setting* settings);
	
	int getSettingInt(const char* key);
	bool getSettingBool(const char* key);
	float getSettingDouble(const char* key);
	std::string getSettingString(const char* key);

	void onTick();
	bool start();
	bool stop();
	Player* createPlayer(Router* router);
	bool removePlayer(unsigned int id);
	World* createWorld();
	bool removeWorld(unsigned int id);
};