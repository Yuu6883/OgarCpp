#pragma once

#include <string>
using std::string;

class Cell;
class World;
class ServerHandle;
class Connection;
class Player;

class Gamemode {
public:
	ServerHandle* handle;
	Gamemode(ServerHandle* handle) : handle(handle) {};

	virtual unsigned char getType() = 0;
	virtual string getName() = 0;
	virtual Gamemode* clone() = 0;

	virtual void onHandleStart() = 0;
	virtual void onHandleTick() = 0;
	virtual void onHandleStop() = 0;

	virtual bool canJoinWorld(World* world) = 0;
	virtual void onNewWorld(World* world) = 0;
	virtual void onWorldTick(World* world) = 0;
	virtual void onWorldDestroy(World* world) = 0;

	virtual void onNewPlayer(Player* player) = 0;
	void onPlayerPressQ(Player* player);
	virtual void onPlayerEject(Player* player) = 0;
	void onPlayerSplit(Player* player);
	virtual void onPlayerSpawnRequest(Player* player, string name, string skin) = 0;
	virtual void onPlayerDestroy(Player* player) = 0;

	virtual void onPlayerJoinWorld(Player* player, World* world) = 0;
	virtual void onPlayerLeaveWorld(Player* player, World* world) = 0;

	virtual void onNewCell(Cell* cell) = 0;
	virtual bool canEat(Cell* a, Cell* b) = 0;
	float getDecayMult(Cell* cell);
	virtual void onCellRemove(Cell* cell) = 0;

	virtual void compileLeaderboard(World* world) = 0;
	virtual void sendLeaderboard(Connection* connection) = 0;
};