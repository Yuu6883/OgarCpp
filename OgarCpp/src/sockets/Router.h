#pragma once

class ServerHandle;
class Listener;
class Player;
class PlayerCell;

#include <regex>
#include "Listener.h"
#include "../ServerHandle.h"
#include "../worlds/Player.h"

static std::regex nameSkinRegex{ "<(.*)>(.*)" };

class Router {
public:
	Listener* listener;

	bool disconnected = false;
	unsigned long disconnectedTick = 0;

	double mouseX = 0;
	double mouseY = 0;

	std::string spawningName = "";
	bool requestingSpectate = false;
	bool isPressingQ = false;
	bool hasPressedQ = false;
	unsigned short splitAttempts = 0;
	unsigned short ejectAttempts = 0;
	unsigned long ejectTick;
	bool hasPlayer = false;
	Player* player = nullptr;
	
	Router(Listener* listener) : listener(listener), ejectTick(listener->handle->tick) {
		this->listener->addRouter(this);
	}

	void createPlayer() {
		if (hasPlayer) return;
		hasPlayer = true;
		player = listener->handle->createPlayer(this);
	}

	void destroyPlayer() {
		if (!hasPlayer) return;
		hasPlayer = false;
		listener->handle->removePlayer(player->id);
		player = nullptr;
	}

	void onWorldSet();
	void onWorldReset();
	void onNewOwnedCell(PlayerCell*);

	void onSpawnRequest() {
		if (!hasPlayer) return;
		int playerMaxNameLength = listener->handle->getSettingInt("playerMaxNameLength");
		std::string name = spawningName.substr(0, playerMaxNameLength);
		std::string skin = "";
		if (listener->handle->getSettingBool("playerAllowSkinInName")) {
			std::smatch sm;
			std::regex_match(name, sm, nameSkinRegex);
			if (sm.size() == 3) {
				skin = sm[1];
				name = sm[2];
			}
		}
		// listener->handle->gamemode->onPlayerSpawnRequest(player, name, skin);
	}
};