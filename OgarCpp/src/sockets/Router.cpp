#include "Router.h"
#include "../ServerHandle.h"

Router::Router(Listener* listener) : listener(listener), ejectTick(listener->handle->tick) {
};

Router::~Router() {
}

void Router::createPlayer() {
	if (hasPlayer) return;
	player = listener->handle->createPlayer(this);
	hasPlayer = true;
};

void Router::destroyPlayer() {
	if (!hasPlayer) return;
	listener->handle->removePlayer(player->id);
	hasPlayer = false;
	player = nullptr;
};

void Router::onSpawnRequest() {
	requestSpawning = false;
	int playerMaxNameLength = listener->handle->runtime.playerMaxNameLength;
	std::string name = spawningName.substr(0, playerMaxNameLength);
	std::string skin = "";
	if (listener->handle->runtime.playerAllowSkinInName) {
		std::smatch sm;
		std::regex_match(name, sm, nameSkinRegex);
		if (sm.size() == 3) {
			skin = sm[1];
			name = sm[2];
		}
		else skin = spawningSkin;
	} else {
		skin = spawningSkin;
	}
	Logger::debug(string("Name: ") + name + " Skin: " + skin);
	listener->handle->gamemode->onPlayerSpawnRequest(player, name, skin);
};

void Router::onSpectateRequest() {
	requestingSpectate = false;
	if (!hasPlayer) return;
	player->updateState(PlayerState::SPEC);
}

void Router::onQPress() {
	if (!hasPlayer) return;
	listener->handle->gamemode->onPlayerPressQ(player);
};

void Router::attemptSplit() {
	if (!hasPlayer) return;
	listener->handle->gamemode->onPlayerSplit(player);
}

void Router::attemptEject() {
	if (!hasPlayer) return;
	listener->handle->gamemode->onPlayerEject(player);
}

void Router::close() {
}