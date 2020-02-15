#include "ServerHandle.h"
#include "worlds/Player.h"
#include "worlds/World.h"

ServerHandle::ServerHandle(Setting* settings) {
	ticker.add([this] { onTick(); });
	setSettings(settings);
};

void ServerHandle::setSettings(Setting* settings) {
	if (this->settings) delete this->settings;

	this->settings = settings;
	int freq = getSettingInt("serverFrequency");
	tickDelay = 1000 / freq;
	ticker.setStep(tickDelay);
	stepMult = tickDelay / 40;
};

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
	listener.open();

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