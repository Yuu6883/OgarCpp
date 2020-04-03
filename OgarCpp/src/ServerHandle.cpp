#include "ServerHandle.h"
#include "Settings.h"
#include "worlds/Player.h"
#include "worlds/World.h"

#define LOAD_INT(prop) runtime.prop = getSettingInt(#prop)
#define LOAD_FLOAT(prop) runtime.prop = getSettingDouble(#prop)
#define LOAD_BOOL(prop) runtime.prop = getSettingBool(#prop)
#define LOAD_STR(prop) runtime.prop = getSettingString(#prop)

ServerHandle::ServerHandle(Setting* settings) {
	ticker.add([this] { onTick(); });
	setSettings(settings);
	protocols = new ProtocolStore();
	gamemodes = new GamemodeList(this);
	gamemode = nullptr;
};

ServerHandle::~ServerHandle() {
	delete gamemodes;
	delete protocols;
	unloadConfig();
}

void ServerHandle::setSettings(Setting* settings) {

	this->settings = settings;
	int freq = getSettingInt("serverFrequency");
	tickDelay = 1000 / freq;
	ticker.setStep(tickDelay);
	stepMult = tickDelay / 40;

	LOAD_BOOL(chatEnabled);
	LOAD_STR(serverName);
	LOAD_INT(worldMaxPlayers);
	LOAD_INT(worldMaxCount);
	LOAD_INT(chatCooldown);
	LOAD_INT(matchmakerBulkSize);
	LOAD_BOOL(minionEnableQBasedControl);
	LOAD_BOOL(matchmakerNeedsQueuing);
	LOAD_FLOAT(minionSpawnSize);
	LOAD_INT(listenerMaxConnections);
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
	LOAD_INT(playerNoMergeDelay);
	LOAD_BOOL(playerMergeNewVersion);
	LOAD_INT(playerMergeTime);
	LOAD_FLOAT(playerMergeTimeIncrease);
	LOAD_FLOAT(playerDecayMult);
}

int ServerHandle::getSettingInt(const char* key) {
	int value = 0;
	settings->lookupValue(key, value);
	return value;
};

bool ServerHandle::getSettingBool(const char* key) {
	bool value = false;
	settings->lookupValue(key, value);
	return value;
};

float ServerHandle::getSettingDouble(const char* key) {
	double value;
	if (settings->lookupValue(key, value))
		return value;
	int v2;
	if (settings->lookupValue(key, v2))
		return (float) v2;
	Logger::error(string("Failed to load prop: ") + key);
	return 0;
};

std::string ServerHandle::getSettingString(const char* key) {
	std::string value = "";
	settings->lookupValue(key, value);
	return value;
};

void ServerHandle::onTick() {
	stopwatch.begin();
	tick++;

	for (auto pair : worlds)
		pair.second->update();
	listener.update();
	matchmaker.update();
	gamemode->onHandleTick();

	averageTickTime = stopwatch.elapsed();
	stopwatch.stop();
};

bool ServerHandle::start() {
	if (running) return false;

	gamemodes->setGamemode(getSettingString("serverGamemode"));

	startTime = system_clock::now();
	averageTickTime = tick = 0;
	running = true;

	listener.open(std::thread::hardware_concurrency());
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
	for (auto pair : worlds)
		removeWorld(pair.first);
	for (auto pair : players)
		removePlayer(pair.first);
	for (auto router : listener.routers)
		router->close();
	gamemode->onHandleStop();
	listener.close();

	running = false;

	Logger::info("Ticker stopped");
	return true;
};

Player* ServerHandle::createPlayer(Router* router) {
	unsigned int id = 0;
	while (players.contains(++id));

	auto player = new Player(this, id, router);
	players.insert(std::make_pair(id, player));
	gamemode->onNewPlayer(player);

	Logger::debug(std::string("Added player with ID ") + std::to_string(id));
	return player;
};

bool ServerHandle::removePlayer(unsigned int id) {
	if (!players.contains(id)) return false;
	auto player = players[id];
	gamemode->onPlayerDestroy(player);
	delete player;
	players.erase(id);
	Logger::debug(std::string("Removed player with ID ") + std::to_string(id));
	return true;
};

World* ServerHandle::createWorld() {
	unsigned int id = 0;
	while (worlds.contains(++id));

	auto world = new World(this, id);
	worlds.insert(std::make_pair(id, world));
	gamemode->onNewWorld(world);
	world->afterCreation();
	Logger::debug(std::string("Added world with ID ") + std::to_string(id));
	return world;
};

bool ServerHandle::removeWorld(unsigned int id) {
	if (!worlds.contains(id)) return false;
	gamemode->onWorldDestroy(worlds[id]);
	delete worlds[id];
	worlds.erase(id);
	Logger::debug(std::string("Removed world with ID ") + std::to_string(id));
	return true;
}