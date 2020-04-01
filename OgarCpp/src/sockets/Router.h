#pragma once

class Listener;
class Player;

#include <regex>
#include "Listener.h"
#include "../cells/Cell.h"
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
	
	Router(Listener* listener);
	virtual bool isExternal();
	void createPlayer();
	void destroyPlayer();
	void onWorldSet();
	void onWorldReset();
	void onNewOwnedCell(PlayerCell*);
	void onSpawnRequest();
	void onSpectateRequest();
	void onQPress();
	void attemptSplit();
	void attemptEject();
	void close();
	bool shouldClose();
	void update();
};