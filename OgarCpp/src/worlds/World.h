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

class PlayerCell;
class Virus;
class EjectedCell;
class Cell;

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
	unsigned long getNextCellId() { return _nextCellId >= 4294967296 ? (_nextCellId = 1) : _nextCellId++; };
	void afterCreation();
	void setBorder(Rect& rect);
	void addCell(Cell* cell);
	void setCellAsBoosting(Cell* cell);
	void setCellAsNotBoosting(Cell* cell);
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
	void resolveEatCheck(Cell* a, Cell* b);
	void boostCell(Cell* cell);
	void bounceCell(Cell* cell, bool bounce);
	void splitVirus(Virus* virus);
	void movePlayerCell(PlayerCell* cell);
	void decayPlayerCell(PlayerCell* cell);
	void launchPlayerCell(PlayerCell* cell, double size, Boost& boost);
	void autosplitPlayerCell(PlayerCell* cell);
	void splitPlayer(Player* player);
	void ejectFromPlayer(Player* player);
	void popPlayerCell(Player* player);
	std::vector<double>* distributeCellMass(PlayerCell* cell);
	void compileStatistics();
};