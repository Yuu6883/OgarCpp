#pragma once

#include <vector>
#include <list>
#include "../sockets/ChatChannel.h"
#include "../primitives/Rect.h"
#include "../primitives/SimplePool.h"

struct SpawnResult {
	unsigned int color;
	Point pos;
};

#include "../primitives/QuadTree.h"
#include "../cells/Cell.h"
#include "Player.h"

struct WorldStats {
	unsigned short limit = 0;
	unsigned short internal = 0;
	unsigned short external = 0;
	unsigned short playing = 0;
	unsigned short spectating = 0;
	string name = "";
	string gamemode = "";
	float loadTime = 0;
	unsigned int uptime = 0;
};

class World : public Spawner {
public:
	ServerHandle* handle;
	ThreadPool* physicsPool;
	unsigned int id;
	bool frozen = false;
	bool toBeRemoved = false;
	unsigned int _nextCellId = 1;
	unsigned ejectCount = 0;
	
	list<Cell*> gcTruck;
	list<Cell*> cells;
	list<Player*> players;
	Player* largestPlayer = nullptr;

	ChatChannel* worldChat;

	bool shouldRestart = false;
	int motherCellCount = 0;
	int virusCount = 0;
	Rect border;

	QuadTree* finder = nullptr;
	QuadTree* lockedFinder = nullptr;

	WorldStats stats;

	World(ServerHandle* handle, unsigned int id);
	~World();
	unsigned int getNextCellId() { return _nextCellId > 4294967295U ? (_nextCellId = 1) : _nextCellId++; };
	void afterCreation();
	void setBorder(Rect& rect);
	void addCell(Cell* cell);
	void updateCell(Cell* cell);
	void removeCell(Cell* cell);
	void addPlayer(Player* player);
	void killPlayer(Player* player, bool instantKill = false);
	void removePlayer(Player* player);
	Point getRandomPos(float cellSize);
	bool isSafeSpawnPos(Rect& range);
	Point getSafeSpawnPos(float& cellSize, bool& failed);
	SpawnResult getPlayerSpawn(float& cellSize, bool& failed);
	void spawnPlayer(Player* player, Point& pos, float size);
	void update() { frozen ? frozenUpdate() : liveUpdate(); };
	void frozenUpdate();
	void liveUpdate();
	void resolveRigidCheck(Cell* a, Cell* b);
	void resolveEatCheck(Cell* a, Cell* b);
	void boostCell(Cell* cell);
	void bounceCell(Cell* cell, bool bounce = false);
	void splitVirus(Virus* virus);
	void movePlayerCell(PlayerCell* cell);
	void decayPlayerCell(PlayerCell* cell);
	void launchPlayerCell(PlayerCell* cell, float size, Boost& boost);
	void autosplitPlayerCell(PlayerCell* cell);
	void splitPlayer(Player* player);
	void ejectFromPlayer(Player* player);
	void popPlayerCell(PlayerCell* cell);
	void distributeCellMass(PlayerCell* cell, std::vector<float>& ref);
	void compileStatistics();
	void clearTruck();
	void restart();
};