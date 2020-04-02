#include "World.h"
#include "../ServerHandle.h"
#include <random>
#include <minmax.h>

using std::to_string;

World::World(ServerHandle* handle, unsigned int id) : handle(handle), id(id) {
	worldChat = new ChatChannel(&handle->listener);
	double x = handle->getSettingDouble("worldMapX");
	double y = handle->getSettingDouble("worldMapY");
	double w = handle->getSettingDouble("worldMapW");
	double h = handle->getSettingDouble("worldMapH");
	Rect rect(x, y, w, h);
	setBorder(rect);
}

void World::afterCreation() {
	int bots = handle->getSettingInt("worldPlayerBotsPerWorld");
	while (bots-- > 0) {
		// TODO: add playerbot
	}
}

World::~World() {
	delete worldChat;
	while (players.size())
		removePlayer(players[0]);
	while (cells.size())
		removeCell(cells[0]);
}

void World::setBorder(Rect& rect) {
	border = rect;
	if (finder) delete finder;
	int maxLevel = handle->getSettingInt("worldFinderMaxLevel");
	int maxItems = handle->getSettingInt("worldFinderMaxItems");
	finder = new QuadTree(border, maxLevel, maxItems);
	for (auto cell : cells) {
		if (cell->getType() == CellType::PLAYER) continue;
		finder->insert(cell);
		if (!border.fullyIntersects(cell->range))
			removeCell(cell);
	}
}

void World::addCell(Cell* cell) {
	cell->exist = true;
	cell->range = { cell->x, cell->y, cell->size, cell->size };
	cells.push_back(cell);
	finder->insert(cell);
	cell->onSpawned();
	handle->gamemode->onNewCell(cell);
}

bool World::setCellAsBoosting(Cell* cell) {
	if (cell->isBoosting) return false;
	cell->isBoosting = true;
	boostingCells.push_back(cell);
	return true;
}

bool World::setCellAsNotBoosting(Cell* cell) {
	if (!cell->isBoosting) return false;
	cell->isBoosting = false;
	auto iter = boostingCells.begin();
	auto cend = boostingCells.cend();
	while (iter < cend) {
		if (*iter == cell) {
			boostingCells.erase(iter);
			return true;
		}
		iter++;
	}
	return true;
}

void World::updateCell(Cell* cell) {
	cell->range = {
		cell->x,
		cell->y,
		cell->size,
		cell->size
	};
	finder->update(cell);
}

void World::removeCell(Cell* cell) {
	handle->gamemode->onCellRemove(cell);
	cell->onRemoved();
	finder->remove(cell);
	setCellAsNotBoosting(cell);
	auto iter = cells.begin();
	auto cend = cells.cend();
	while (iter < cend) {
		if (*iter == cell) {
			cells.erase(iter); 
			break;
		}
		iter++;
	}
	delete cell;
}

void World::addPlayer(Player* player) {
	players.push_back(player);
	player->world = this;
	player->hasWorld = true;
	worldChat->add(player->router);
	Logger::debug(string("player ") + to_string(player->id) + " has been added to world " + to_string(id));
	if (!player->router->isExternal()) return;
	int minionsPerPlayer = handle->getSettingInt("worldMinionsPerPlayer");
	while (minionsPerPlayer-- > 0) {
		// TODO: add minions
	}
}

void World::removePlayer(Player* player) {
	auto iter = players.begin();
	auto cend = players.cend();
	while (iter < cend) {
		if (*iter == player) {
			players.erase(iter);
			break;
		}
	}
	handle->gamemode->onPlayerLeaveWorld(player, this);
	player->world = nullptr;
	player->hasWorld = false;
	worldChat->remove(player->router);
	while (player->ownedCells.size())
		removeCell(player->ownedCells[0]);
	player->router->onWorldReset();
	Logger::debug(string("player ") + to_string(player->id) + " has been removed from world " + to_string(id));
};

Point World::getRandomPos(double cellSize) {
	return {
		border.x - border.w + cellSize + ((double)rand() / (RAND_MAX)) * (2 * border.w - cellSize),
		border.y - border.h + cellSize + ((double)rand() / (RAND_MAX)) * (2 * border.h - cellSize)
	};
}

bool World::isSafeSpawnPos(Rect& range) {
	return !finder->containAny(range, [](auto item) { return ((Cell*) item)->shouldAvoidWhenSpawning(); });
}

Point World::getSafeSpawnPos(double cellSize) {
	int tries = handle->getSettingInt("worldSafeSpawnTries");
	while (--tries >= 0) {
		auto pos = getRandomPos(cellSize);
		Rect rect(pos.x, pos.y, cellSize, cellSize);
		if (isSafeSpawnPos(rect))
			return Point(pos);
	}
	return Point(getRandomPos(cellSize));
}

SpawnResult World::getPlayerSpawn(double cellSize) {
	double rnd = (double)rand() / (RAND_MAX);
	double chance = handle->getSettingDouble("worldSafeSpawnFromEjectedChance");
	if (chance > rnd && ejectedCells.size() > 0) {
		int tries = handle->getSettingInt("worldSafeSpawnTries");
		while (--tries >= 0) {
			std::random_device random_device;
			std::mt19937 engine{ random_device() };
			std::uniform_int_distribution<int> dist(0, ejectedCells.size() - 1);
			auto cell = ejectedCells[dist(engine)];
			Rect rect(cell->x, cell->y, cellSize, cellSize);
			if (isSafeSpawnPos(rect)) {
				removeCell(cell);
				return { cell->color, { cell->x, cell->y } };
			}
		}
		return { (unsigned int) -1, getSafeSpawnPos(cellSize) };
	}
}

void World::spawnPlayer(Player*, Point& pos, double size) {
	// TODO: Make new player cell
	// TODO: Call addCell
	// TODO: update player state
}

void World::frozenUpdate() {
	for (auto player : this->players) {
		auto router = player->router;
		router->splitAttempts = 0;
		router->ejectAttempts = 0;
		if (router->isPressingQ) {
			if (!router->hasPressedQ)
				router->onQPress();
			router->hasPressedQ = true;
		} else router->hasPressedQ = false;
		router->requestingSpectate = false;
		router->spawningName = "";
	}
}

void World::liveUpdate() {
	handle->gamemode->onWorldTick(this);

	auto rigid = new vector<Cell*>;
	auto eat   = new vector<Cell*> ;

	for (auto c : cells)
		c->onTick();

	// TODO: Add pellet
	// TODO: Add virus
	// TODO: Add mothercell
	
	for (int i = 0, l = boostingCells.size(); i < l;) {
		if (!boostCell(boostingCells[i])) l--;
		else i++;
	}

	for (auto c : boostingCells) {
		if (c->getType() != CellType::VIRUS && c->getType() != CellType::EJECTED_CELL) continue;
		finder->search(c->range, [c, rigid, eat](auto o) {
			auto other = (Cell*) o;
			if (c->id == other->id) return;
			switch (c->getEatResult(other)) {
				case EatResult::COLLIDE: 
					rigid->push_back(c);
					rigid->push_back(other);
					break;
				case EatResult::EAT:
					eat->push_back(c);
					eat->push_back(other);
					break;
				case EatResult::EATINVD:
					eat->push_back(other);
					eat->push_back(c);
					break;
			}
		});
	}

	double playerMinSize = handle->getSettingDouble("playerMinSize");

	for (auto c : playerCells) {
		movePlayerCell(c);
		decayPlayerCell(c);
		autosplitPlayerCell(c);
		bounceCell(c);
		updateCell(c);
	}

	for (auto c : playerCells) {
		finder->search(c->range, [c, rigid, eat](auto o) {
			auto other = (Cell*)o;
			if (c->id == other->id) return;
			switch (c->getEatResult(other)) {
				case EatResult::COLLIDE:
					rigid->push_back(c);
					rigid->push_back(other);
					break;
				case EatResult::EAT:
					eat->push_back(c);
					eat->push_back(other);
					break;
				case EatResult::EATINVD:
					eat->push_back(other);
					eat->push_back(c);
					break;
			}
		});
	}

	for (int i = 0, l = rigid->size(); i < l;)
		resolveRegidCheck(rigid->at(i++), rigid->at(i++));
	for (int i = 0, l = eat->size(); i < l;)
		resolveEatCheck(eat->at(i++), eat->at(i++));

	delete rigid;
	delete eat;

	largestPlayer = nullptr;
	for (auto p : players)
		if (p->score > 0 && (!largestPlayer || p->score > largestPlayer->score))
			largestPlayer = p;

	int splitCap = handle->getSettingInt("playerSplitCap");
	int ejectDelay = handle->getSettingInt("playerEjectDelay");

	for (int i = 0, l = players.size(); i < l; i++) {

		auto player = players[i];
		player->checkExistence();
		if (!player->exists) { i--; l--; continue; }

		if (player->state == PlayerState::SPEC && !largestPlayer)
			player->updateState(PlayerState::ROAM);

		auto router = player->router;
		for (int j = 0, k = splitCap; j < k && router->splitAttempts > 0; j++) {
			router->attemptSplit();
			router->splitAttempts--;
		}

		auto nextEjectTick = handle->tick - ejectDelay;
		if (router->ejectAttempts > 0 && nextEjectTick >= router->ejectTick) {
			router->attemptEject();
			router->ejectAttempts = 0;
			router->ejectTick = handle->tick;
		}

		if (router->isPressingQ) {
			if (!router->hasPressedQ)
				router->onQPress();
			router->hasPressedQ = true;
		} else router->hasPressedQ = false;

		if (router->requestingSpectate) {
			router->onSpawnRequest();
			router->requestingSpectate = false;
		}

		if (router->spawningName.size()) {
			router->onSpawnRequest();
			router->spawningName = "";
		}

		player->updateViewArea();
	}

	compileStatistics();
	handle->gamemode->compileLeaderboard(this);

	if (stats.external <= 0) {
		if (handle->worlds.size() > handle->getSettingInt("worldMinCount"))
			handle->removeWorld(id);
	}
}

void World::resolveRegidCheck(Cell* a, Cell* b) {
	double dx = b->x - a->x;
	double dy = b->y - a->y;
	double d = sqrt(dx * dx + dy * dy);
	double m = a->size + b->size - d;
	if (m <= 0) return;
	if (!d) d = 1, dx = 1, dy = 0;
	else dx /= d, dy /= d;
	double M = a->getSquareSize() + b->getSquareSize();
	double aM = b->getSquareSize() / M;
	double bM = b->getSquareSize() / M;
	a->x -= dx * m * aM;
	a->y -= dy * m * aM;
	b->x += dx * m * bM;
	b->y += dy * m * bM;
	bounceCell(a);
	bounceCell(b);
	updateCell(a);
	updateCell(b);
}

void World::resolveEatCheck(Cell* a, Cell* b) {
	if (!a->exist || !b->exist) return;
	double dx = b->x - a->x;
	double dy = b->y - a->y;
	double d = sqrt(dx * dx + dy * dy);
	if (d > a->size - b->size / handle->runtime.worldEatOverlapDiv) return;
	if (!handle->gamemode->canEat(a, b)) return;
	a->whenAte(b);
	b->whenEatenBy(a);
	removeCell(b);
	updateCell(a);
}

bool World::boostCell(Cell* cell) {
	double d = cell->boost.d / 9 * handle->stepMult;
	cell->x += cell->boost.dx * d;
	cell->y += cell->boost.dy * d;
	bounceCell(cell, true);
	updateCell(cell);
	if ((cell->boost.d -= d) >= 1) return true;
	setCellAsNotBoosting(cell);
	return false;
}

void World::bounceCell(Cell* cell, bool bounce) {
	double r = cell->size / 2.0;
	if (cell->x <= border.x - border.w + r) {
		cell->x = border.x - border.w + r;
		if (bounce) cell->boost.dx = -cell->boost.dx;
	}
	if (cell->x >= border.x + border.w - r) {
		cell->x = border.x + border.w - r;
		if (bounce) cell->boost.dx = -cell->boost.dx;
	}
	if (cell->y <= border.y - border.h + r) {
		cell->y = border.y - border.h + r;
		if (bounce) cell->boost.dy = -cell->boost.dy;
	}
	if (cell->y >= border.y + border.h - r) {
		cell->y = border.y + border.h - r;
		if (bounce) cell->boost.dy = -cell->boost.dy;
	}
}

void World::splitVirus(Virus* virus) {
	// TODO
}

void World::movePlayerCell(PlayerCell* cell) {
	// TODO
}

void World::decayPlayerCell(PlayerCell* cell) {
	double newSize = cell->size - cell->size * handle->gamemode->getDecayMult(cell) / 50 * handle->stepMult;
	double minSize = handle->runtime.playerMinSize;
	cell->size = max(newSize, minSize);
}

void World::launchPlayerCell(PlayerCell* cell, double size, Boost& boost) {
	// TODO
}

void World::autosplitPlayerCell(PlayerCell* cell) {
	// TODO
}

void World::splitPlayer(Player* player) {
	auto router = player->router;
	// for (auto c : player->ownedCells) {
		// TODO
	// }
}

void World::ejectFromPlayer(Player* player) {
	// TODO
}

void World::popPlayerCell(PlayerCell* cell) {
	// TODO
}

void World::distributeCellMass(PlayerCell* cell, std::vector<double>& ref) {
	// TODO
}

void World::compileStatistics() {
	unsigned int internal = 0, external = 0, playing = 0, spectating = 0;
	for (auto p : players) {
		if (!p->router->isExternal()) { internal++; continue; }
		external++;
		if (p->state == PlayerState::ALIVE) playing++;
		else if (p->state == PlayerState::SPEC || p->state == PlayerState::ROAM)
			spectating++;
	}
	// TODO: update stats
}

