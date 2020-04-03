#pragma once

#include <vector>
#include "Gamemode.h"

using std::vector;

class FFA : public Gamemode {
public:
	vector<Player*> leaderboard;
	FFA(ServerHandle* handle) : Gamemode(handle) {};
	unsigned char getType() { return 0; };
	string getName() { return "FFA"; };

	void onHandleStart() {};
	void onHandleTick() {};
	void onHandleStop() {};

	bool canJoinWorld(World* world) { return true; };
	void onNewWorld(World* world) {};
	void onWorldTick(World* world) {};
	void onWorldDestroy(World* world) {};

	void onNewPlayer(Player* player) {};
	void onPlayerEject(Player* player) {};

	void onPlayerDestroy(Player* player) {};

	void onPlayerJoinWorld(Player* player, World* world) {};
	void onPlayerLeaveWorld(Player* player, World* world) {};

	void onNewCell(Cell* cell) {};
	bool canEat(Cell* a, Cell* b) { return true; };
	void onCellRemove(Cell* cell) {};

	void onPlayerSpawnRequest(Player* player, string name, string skin);
	void compileLeaderboard(World* world);
	void sendLeaderboard(Connection* connection);
	Gamemode* clone() {
		return new FFA(*this);
	}
};