#pragma once

#include "../ServerHandle.h"
#include "../worlds/World.h"
#include "../cells/Cell.h"

class ServerHandle;
class Connection;
class Player;
class PlayerCell;

class Gamemode {
public:
	ServerHandle* handle;
	Gamemode(ServerHandle* handle) : handle(handle) {};
	int getType();
	const char* getName();

	void onHandleStart();
	void onHandleTick();
	void onHandleStop();

	bool canJoinWorld(World* world);
	void onNewWorld(World* world);
	void onWorldTick(World* world);
	void onWorldDestroy(World* world);

	void onNewPlayer(Player* player);
	void onPlayerPressQ(Player* player);
	void onPlayerEject(Player* player);
	void onPlayerSplit(Player* player);
	void onPlayerRequestSpawn(Player* player);
	void onPlayerDestroy(Player* player);

	void onPlayerJoinWorld(Player* player, World* world);
	void onPlayerLeaveWorld(Player* player, World* world);

	void onNewCell(Cell* cell);
	bool canEat(Cell* a, Cell* b);
	double getDecayMult(Cell* cell);
	void onCellRemove(Cell* cell);

	void compileLeaderboard(World* world);
	void sendLeaderboard(Connection* connection);
};