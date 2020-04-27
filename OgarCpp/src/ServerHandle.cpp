#include "ServerHandle.h"
#include "Settings.h"
#include "worlds/Player.h"
#include "worlds/World.h"

using std::to_string;

#define LOAD_INT(prop) runtime.prop = getSettingInt(#prop)
#define LOAD_FLOAT(prop) runtime.prop = getSettingFloat(#prop)
#define LOAD_BOOL(prop) runtime.prop = getSettingBool(#prop)
#define LOAD_STR(prop) runtime.prop = getSettingString(#prop)

ServerHandle::ServerHandle() {
	loadSettings();
	ticker.add([this] { onTick(); });
	protocols = new ProtocolStore();
	gamemodes = new GamemodeList(this);
	gamemode = nullptr;
};

ServerHandle::~ServerHandle() {
	if (running) stop();
	delete gamemodes;
	delete protocols;
	while (worlds.size())
		removeWorld(worlds.begin()->first);
	FREE_QUADTREES();
}

void ServerHandle::loadSettings() {

	loadConfig();
	int freq = getSettingInt("serverFrequency");
	tickDelay = 1000 / freq;
	ticker.setStep(tickDelay);
	stepMult = tickDelay / 40;

	LOAD_BOOL(chatEnabled);
	LOAD_STR(serverName);
	LOAD_INT(worldMaxPlayers);
	LOAD_INT(worldMinCount);
	LOAD_INT(worldMaxCount);
	LOAD_INT(physicsThreads);
	LOAD_BOOL(respawnEnabled);
	LOAD_INT(chatCooldown);
	LOAD_INT(matchmakerBulkSize);
	LOAD_BOOL(minionEnableQBasedControl);
	LOAD_BOOL(matchmakerNeedsQueuing);
	LOAD_FLOAT(minionSpawnSize);
	LOAD_INT(listenerMaxConnections);
	LOAD_INT(listenerMaxConnectionsPerIP);
	LOAD_FLOAT(worldEatMult);
	LOAD_FLOAT(worldEatOverlapDiv);
	LOAD_INT(worldSafeSpawnTries);
	LOAD_FLOAT(worldSafeSpawnFromEjectedChance);
	LOAD_INT(worldPlayerDisposeDelay);
	LOAD_INT(pelletMinSize);
	LOAD_INT(pelletMaxSize);
	LOAD_INT(pelletGrowTicks);
	LOAD_INT(pelletCount);
	LOAD_INT(virusMinCount);
	LOAD_INT(virusMaxCount);
	LOAD_FLOAT(virusSize);
	LOAD_INT(virusFeedTimes);
	LOAD_BOOL(virusPushing);
	LOAD_FLOAT(virusSplitBoost);
	LOAD_FLOAT(virusPushBoost);
	LOAD_BOOL(virusMonotonePops);
	LOAD_FLOAT(ejectedSize);
	LOAD_FLOAT(ejectingLoss);
	LOAD_FLOAT(ejectDispersion);
	LOAD_FLOAT(ejectedCellBoost);
	LOAD_FLOAT(mothercellSize);
	LOAD_INT(mothercellCount);
	LOAD_FLOAT(mothercellPassiveSpawnChance);
	LOAD_FLOAT(mothercellActiveSpawnSpeed);
	LOAD_FLOAT(mothercellPelletBoost);
	LOAD_INT(mothercellMaxPellets);
	LOAD_FLOAT(mothercellMaxSize);
	LOAD_FLOAT(playerRoamSpeed);
	LOAD_FLOAT(playerRoamViewScale);
	LOAD_FLOAT(playerViewScaleMult);
	LOAD_FLOAT(playerMinViewScale);
	LOAD_INT(playerMaxNameLength);
	LOAD_BOOL(playerAllowSkinInName);
	LOAD_FLOAT(playerMinSize);
	LOAD_FLOAT(playerSpawnSize);
	LOAD_FLOAT(playerMaxSize);
	LOAD_FLOAT(playerMinSplitSize);
	LOAD_FLOAT(playerMinEjectSize);
	LOAD_INT(playerSplitCap);
	LOAD_INT(playerEjectDelay);
	LOAD_INT(playerMaxCells);
	LOAD_FLOAT(playerMoveMult);
	LOAD_FLOAT(playerSplitSizeDiv);
	LOAD_FLOAT(playerSplitDistance);
	LOAD_FLOAT(playerSplitBoost);
	LOAD_FLOAT(playerNoCollideDelay);
	LOAD_FLOAT(playerNoMergeDelay);
	LOAD_BOOL(playerMergeNewVersion);
	LOAD_FLOAT(playerMergeTime);
	LOAD_FLOAT(playerMergeTimeIncrease);
	LOAD_FLOAT(playerDecayMult);
}

int ServerHandle::getSettingInt(const char* key) {
	int value = 0;
	if (!GAME_CONFIG[key].is_number_integer()) {
		Logger::warn(string("Failed to get integer from config (key: ") + key + ")");
	} else {
		value = GAME_CONFIG[key];
	}
	return value;
};

bool ServerHandle::getSettingBool(const char* key) {
	bool value = false;
	if (!GAME_CONFIG[key].is_boolean()) {
		Logger::warn(string("Failed to get bool from config (key: ") + key + ")");
	} else {
		value = GAME_CONFIG[key];
	}
	return value;
};

float ServerHandle::getSettingFloat(const char* key) {
	float value = 0;
	if (GAME_CONFIG[key].is_number_integer()) {
		value = (int) GAME_CONFIG[key];
	} else if (GAME_CONFIG[key].is_number_float()) {
		value = GAME_CONFIG[key];
	} else {
		Logger::warn(string("Failed to get float from config (key: ") + key + ")");
	}
	return value;
};

std::string ServerHandle::getSettingString(const char* key) {
	std::string value = "";
	if (!GAME_CONFIG[key].is_string()) {
		Logger::warn(string("Failed to get string from config (key: ") + key + ")");
	} else {
		value = GAME_CONFIG[key];
	}
	return value;
};

void ServerHandle::onTick() {
	stopwatch.begin();
	tick++;

	vector<unsigned int> removingIds;
	for (auto [id, world] : worlds) {
		world->update();
		if (world->toBeRemoved)
			removingIds.push_back(id);
	}

	for (auto id : removingIds)
		removeWorld(id);

	listener.update();
	matchmaker.update();
	gamemode->onHandleTick();

	for (auto [_, world] : worlds)
		world->clearTruck();

	chatCommands.process();
	commands.process();

	averageTickTime = stopwatch.elapsed();
	stopwatch.stop();
};

bool ServerHandle::start() {
	if (running) return false;

	gamemodes->setGamemode(getSettingString("serverGameMode"));

	startTime = system_clock::now();
	averageTickTime = tick = 0;
	running = true;

	listener.open(std::min(getSettingInt("listenerThreads"), (int) std::thread::hardware_concurrency()));
	ticker.start();
	gamemode->onHandleStart();

	Logger::info("Ticker started");
	Logger::info(string("OgarCpp ") + OGAR_VERSION_STRING);
	Logger::info(string("Gamemode: ") + gamemode->getName());
	return true;
};

bool ServerHandle::stop() {
	if (!running) return false;

	if (ticker.running)
		ticker.stop();
	Logger::debug(string("Removing ") + to_string(worlds.size()) + " worlds");
	while (worlds.size())
		removeWorld(worlds.begin()->first);
	for (auto router : listener.routers)
		router->close();
	Logger::debug(string("Removing ") + to_string(players.size()) + " players");
	while (players.size())
		removePlayer(players.begin()->first);
	gamemode->onHandleStop();
	Logger::debug("Closing listeners");
	listener.close();
	running = false;

	Logger::info("Ticker stopped");
	return true;
};

Player* ServerHandle::createPlayer(Router* router) {
	unsigned int id = 0;
	while (players.find(++id) != players.cend());

	auto player = new Player(this, id, router);
	players.insert(std::make_pair(id, player));
	gamemode->onNewPlayer(player);

	Logger::debug(std::string("Added player with ID ") + std::to_string(id));
	return player;
};

bool ServerHandle::removePlayer(unsigned int id) {
	if (players.find(id) == players.cend()) return false;
	auto player = players[id];
	gamemode->onPlayerDestroy(player);
	delete player;
	players.erase(id);
	Logger::debug(std::string("Removed player with ID ") + std::to_string(id));
	return true;
};

World* ServerHandle::createWorld() {
	unsigned int id = 0;
	while (worlds.find(++id) != worlds.cend());

	auto world = new World(this, id);
	worlds.insert(std::make_pair(id, world));
	gamemode->onNewWorld(world);
	world->afterCreation();
	Logger::debug(std::string("Added world with ID ") + std::to_string(id));
	return world;
};

bool ServerHandle::removeWorld(unsigned int id) {
	if (worlds.find(id) == worlds.cend()) return false;
	gamemode->onWorldDestroy(worlds[id]);
	delete worlds[id];
	worlds.erase(id);
	if (!worlds.size()) {
		worlds.clear();
		FREE_QUADTREES();
	}
	Logger::debug(std::string("Removed world with ID ") + std::to_string(id));
	return true;
}