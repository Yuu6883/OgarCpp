#include "ServerHandle.h"
#include "worlds/Player.h"
#include "worlds/World.h"

#define LOAD_INT(prop) runtime.prop = getSettingInt(#prop)
#define LOAD_DOUBLE(prop) runtime.prop = getSettingDouble(#prop)
#define LOAD_BOOL(prop) runtime.prop = getSettingBool(#prop)
#define LOAD_STR(prop) runtime.prop = getSettingString(#prop)

ServerHandle::ServerHandle(Setting* settings) {
	ticker.add([this] { onTick(); });
	setSettings(settings);
	protocols = new ProtocolStore();
	gamemodes = new GamemodeList(this);
	gamemode = nullptr;
};

void ServerHandle::setSettings(Setting* settings) {

	this->settings = settings;
	int freq = getSettingInt("serverFrequency");
	tickDelay = 1000 / freq;
	ticker.setStep(tickDelay);
	stepMult = tickDelay / 40;

	LOAD_STR(serverName);
	LOAD_INT(listenerMaxConnections);
	LOAD_DOUBLE(worldEatMult);
	LOAD_DOUBLE(worldEatOverlapDiv);
	LOAD_INT(worldSafeSpawnTries);
	LOAD_DOUBLE(worldSafeSpawnFromEjectedChance);
	LOAD_INT(worldPlayerDisposeDelay);
	LOAD_INT(pelletMinSize);
	LOAD_INT(pelletMaxSize);
	LOAD_INT(pelletGrowTicks);
	LOAD_INT(pelletCount);
	LOAD_INT(virusMinCount);
	LOAD_INT(virusMaxCount);
	LOAD_DOUBLE(virusSize);
	LOAD_INT(virusFeedTimes);
	LOAD_BOOL(virusPushing);
	LOAD_DOUBLE(virusSplitBoost);
	LOAD_DOUBLE(virusPushBoost);
	LOAD_BOOL(virusMonotonePops);
	LOAD_DOUBLE(ejectedSize);
	LOAD_DOUBLE(ejectingLoss);
	LOAD_DOUBLE(ejectDispersion);
	LOAD_DOUBLE(ejectedCellBoost);
	LOAD_DOUBLE(mothercellSize);
	LOAD_INT(mothercellCount);
	LOAD_DOUBLE(mothercellPassiveSpawnChance);
	LOAD_DOUBLE(mothercellActiveSpawnSpeed);
	LOAD_DOUBLE(mothercellPelletBoost);
	LOAD_INT(mothercellMaxPellets);
	LOAD_DOUBLE(mothercellMaxSize);
	LOAD_DOUBLE(playerRoamSpeed);
	LOAD_DOUBLE(playerRoamViewScale);
	LOAD_DOUBLE(playerViewScaleMult);
	LOAD_DOUBLE(playerMinViewScale);
	LOAD_INT(playerMaxNameLength);
	LOAD_BOOL(playerAllowSkinInName);
	LOAD_DOUBLE(playerMinSize);
	LOAD_DOUBLE(playerSpawnSize);
	LOAD_DOUBLE(playerMaxSize);
	LOAD_DOUBLE(playerMinSplitSize);
	LOAD_DOUBLE(playerMinEjectSize);
	LOAD_INT(playerSplitCap);
	LOAD_INT(playerEjectDelay);
	LOAD_INT(playerMaxCells);
	LOAD_DOUBLE(playerMoveMult);
	LOAD_DOUBLE(playerSplitSizeDiv);
	LOAD_DOUBLE(playerSplitDistance);
	LOAD_DOUBLE(playerSplitBoost);
	LOAD_DOUBLE(playerNoCollideDelay);
	LOAD_INT(playerNoMergeDelay);
	LOAD_BOOL(playerMergeNewVersion);
	LOAD_INT(playerMergeTime);
	LOAD_DOUBLE(playerMergeTimeIncrease);
	LOAD_DOUBLE(playerDecayMult);
}

int ServerHandle::getSettingInt(const char* key) {
	int value;
	settings->lookupValue(key, value);
	return value;
};

bool ServerHandle::getSettingBool(const char* key) {
	bool value;
	settings->lookupValue(key, value);
	return value;
};

double ServerHandle::getSettingDouble(const char* key) {
	double value;
	settings->lookupValue(key, value);
	return value;
};

std::string ServerHandle::getSettingString(const char* key) {
	std::string value;
	settings->lookupValue(key, value);
	return value;
};

void ServerHandle::onTick() {

};

bool ServerHandle::start() {
	if (running) return false;

	running = true;
	ticker.start();
	listener.open(std::thread::hardware_concurrency());

	return true;
};

bool ServerHandle::stop() {
	if (!running) return false;

	if (ticker.running)
		ticker.stop();

	// TODO: remove worlds
	// TODO: remove players
	listener.close();

	startTime = system_clock::now();
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