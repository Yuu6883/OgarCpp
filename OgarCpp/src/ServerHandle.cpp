#include "ServerHandle.h"

ServerHandle::ServerHandle(Setting* settings) {
	
	commands = new CommandList<ServerHandle*>(this);
	chatCommands = new CommandList<ServerHandle*>(this);

	ticker.add(bind(&ServerHandle::onTick, this));

	setSettings(settings);
}

void ServerHandle::setSettings(Setting* settings) {
	if (this->settings) delete this->settings;

	int freq;
	settings->lookupValue("serverFrequency", freq);
	tickDelay = 1000 / freq;
	ticker.setStep(tickDelay);
	stepMult = tickDelay / 40;

	this->settings = settings;
}

int ServerHandle::getSettingInt(const char* key) {
	int value;
	settings->lookupValue(key, value);
	return value;
}

bool ServerHandle::getSettingBool(const char* key) {
	bool value;
	settings->lookupValue(key, value);
	return value;
}

double ServerHandle::getSettingDouble(const char* key) {
	double value;
	settings->lookupValue(key, value);
	return value;
}

string ServerHandle::getSettingString(const char* key) {
	string value;
	settings->lookupValue(key, value);
	return value;
}

void ServerHandle::onTick() {

}

bool ServerHandle::start() {
	if (running) return false;

	running = true;
	ticker.start();

	return true;
}

bool ServerHandle::stop() {
	if (!running) return false;

	if (ticker.running)
		ticker.stop();

	// TODO: remove worlds
	// TODO: remove players
	// TODO: close listeners

	startTime = system_clock::now();
	return true;
}