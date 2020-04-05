#pragma once
#include <regex>
#include <atomic>

using std::atomic;

class Listener;
class Player;
class PlayerCell;

static std::regex nameSkinRegex{ "<(.*)>(.*)" };

enum class RouterType {
	NONE, PLAYER, MINION
};

class Router {
public:
	Listener* listener;

	bool disconnected = false;
	unsigned long disconnectedTick = 0;
	RouterType type = RouterType::NONE;

	std::string spawningName = "";

	atomic<float> mouseX = 0;
	atomic<float> mouseY = 0;
	atomic<bool> requestingSpectate = false;
	atomic<bool> isPressingQ = false;
	atomic<bool> hasPressedQ = false;
	atomic<unsigned short> splitAttempts = 0;
	atomic<unsigned short> ejectAttempts = 0;

	unsigned long ejectTick;
	bool hasPlayer = false;
	Player* player = nullptr;
	
	Router(Listener* listener);
	~Router();
	virtual bool isExternal() = 0;
	void createPlayer();
	void destroyPlayer();
	virtual void onWorldSet() = 0;
	virtual void onWorldReset() = 0;
	virtual void onNewOwnedCell(PlayerCell*) = 0;
	void onQPress();
	void onSpawnRequest();
	void onSpectateRequest();
	void attemptSplit();
	void attemptEject();
	void close();
	virtual bool shouldClose() = 0;
	virtual void update() = 0;
};