#pragma once

#include "../sockets/Router.h"
#include "../worlds/World.h"
#include "../cells/Cell.h"

class PlayerBot : public Router {
public:
	unsigned int splitCooldownTicks = 0;
	unsigned int lockTicks = 0;
	Cell* target = nullptr;
	bool selfeed = false;
	bool trypopsplit = false;
	bool revpopsplit = false;
	PlayerBot(World* world);
	bool shouldClose() {
		return !hasPlayer || !player->exist() || !player->hasWorld;
	};
	bool isExternal() { return false; };
	void close();
	void onWorldSet() {};
	void onWorldReset() {};
	void onNewOwnedCell(PlayerCell*) {};
	void update();
	bool canEat(float aSize, float bSize);
	bool canSplitKill(float aSize, float bSize, float d);
	bool isThreaded() { return false; };
	void onDead();
};