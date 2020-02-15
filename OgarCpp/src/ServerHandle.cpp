#include "ServerHandle.h"

ServerHandle::ServerHandle(Setting* settings) {
	
	commands.handle = this;
	chatCommands.handle = this;

	ticker.add([this] { onTick(); });

	setSettings(settings);
}

void ServerHandle::setSettings(Setting* settings) {
	if (this->settings) delete this->settings;

	this->settings = settings;
	int freq = getSettingInt("serverFrequency");
	tickDelay = 1000 / freq;
	ticker.setStep(tickDelay);
	stepMult = tickDelay / 40;
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

std::string ServerHandle::getSettingString(const char* key) {
	std::string value;
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