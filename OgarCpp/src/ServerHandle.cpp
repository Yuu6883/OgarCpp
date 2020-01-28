#include "ServerHandle.h"

ServerHandle::ServerHandle(Setting* settings) {
	this->settings = settings;
	this->commands = new CommandList<ServerHandle*>(this);
	this->chatCommands = new CommandList<ServerHandle*>(this);

	this->setSettings(settings);
}

void ServerHandle::setSettings(Setting* settings) {
	settings->lookupValue("serverFrequency", this->tick);
}