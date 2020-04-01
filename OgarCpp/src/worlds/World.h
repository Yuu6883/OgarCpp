#pragma once

#include <vector>
#include "../sockets/ChatChannel.h"
#include "../primitives/Rect.h"

struct SpawnResult {
	unsigned int color;
	Point pos;
};

#include "../primitives/QuadTree.h"
#include "../cells/Cell.h"
#include "Player.h"

static struct WorldStats {
	unsigned short limit = 0;
	unsigned short internal = 0;
	unsigned short external = 0;
	unsigned short spectating = 0;
	string name = "";
	string mode = "";
	double loadTime = 0;
	unsigned long uptime = 0;
};

class World {
public:
	ServerHandle* handle;
	unsigned int id;
	bool frozen = false;
	unsigned int _nextCellId = 1;
	
	std::vector<Cell*> cells;
	std::vector<Cell*> boostingCells;
	std::vector<EjectedCell*> ejectedCells;
	std::vector<PlayerCell*> playerCells;
	std::vector<Player*> players;
	Player* largestPlayer = nullptr;

	ChatChannel worldChat = ChatChannel(&handle->listener);

	int pelletCount = 0;
	int motherCellCount = 0;
	int virusCount = 0;
	Rect border;
	QuadTree* finder = nullptr;

	WorldStats stats;

	World(ServerHandle* handle, unsigned int id);
	~World();
	unsigned int getNextCellId() { return _nextCellId > 4294967295U ? (_nextCellId = 1) : _nextCellId++; };
	void afterCreation();
	void setBorder(Rect& rect);
	void addCell(Cell* cell);
	bool setCellAsBoosting(Cell* cell);
	bool setCellAsNotBoosting(Cell* cell);
	void updateCell(Cell* cell);
	void removeCell(Cell* cell);
	void addPlayer(Player* player);
	void removePlayer(Player* player);
	Point getRandomPos(double cellSize);
	bool isSafeSpawnPos(Rect& range);
	Point getSafeSpawnPos(double cellSize);
	SpawnResult getPlayerSpawn(double cellSize);
	void spawnPlayer(Player* player, Point& pos, double size);
	void update() { frozen ? frozenUpdate() : liveUpdate(); };
	void frozenUpdate();
	void liveUpdate();
	void resolveRegidCheck(Cell* a, Cell* b);
	void resolveEatCheck(Cell* a, Cell* b, double);
	bool boostCell(Cell* cell);
	void bounceCell(Cell* cell, bool bounce = false);
	void splitVirus(Virus* virus);
	void movePlayerCell(PlayerCell* cell);
	void decayPlayerCell(PlayerCell* cell, double minSize);
	void launchPlayerCell(PlayerCell* cell, double size, Boost& boost);
	void autosplitPlayerCell(PlayerCell* cell);
	void splitPlayer(Player* player);
	void ejectFromPlayer(Player* player);
	void popPlayerCell(Player* player);
	void distributeCellMass(PlayerCell* cell, std::vector<double>& ref);
	void compileStatistics();
};