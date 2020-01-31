#pragma once

#include <vector>
#include "../primitives/QuadTree.h"
#include "../ServerHandle.h"
#include "../cells/Cell.h"

class ServerHandle;
class Cell;

class World {
public:
	int id;
	bool frozen = false;
	int _nextCellId = 1;
	vector<Cell*> cells;
	vector<Cell*> boostingCells;
	int pelletCount = 0;
	int motherCellCount = 0;
	int virusCount = 0;
	Rect* border;
	QuadTree* finder;
	World(ServerHandle* handle, int id) : id(id) {

	}
};