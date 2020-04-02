#pragma once
#include <regex>

class Listener;
class Player;
class PlayerCell;

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
	virtual bool isExternal() { return false; };
	virtual void createPlayer();
	virtual void destroyPlayer();
	virtual void onWorldSet() {};
	virtual void onWorldReset() {};
	virtual void onNewOwnedCell(PlayerCell*) {};
	void onSpawnRequest();
	void onSpectateRequest();
	void onQPress();
	void attemptSplit();
	void attemptEject();
	void close();
	virtual bool shouldClose() { return false; };
	virtual void update() {};
};