#include "World.h"
#include "../sockets/Router.h"
#include "../ServerHandle.h"
#include "../cells/Cell.h"
#include <random>

using std::to_string;

World::World(ServerHandle* handle, unsigned int id) : handle(handle), id(id) {
	worldChat = new ChatChannel(&handle->listener);
	float x = handle->getSettingInt("worldMapX");
	float y = handle->getSettingInt("worldMapY");
	float w = handle->getSettingInt("worldMapW");
	float h = handle->getSettingInt("worldMapH");
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
		removePlayer(players.front());
	while (cells.size())
		removeCell(cells.front());
}

void World::setBorder(Rect& rect) {
	border = rect;
	if (finder) delete finder;
	int maxLevel = handle->getSettingInt("worldFinderMaxLevel");
	int maxItems = handle->getSettingInt("worldFinderMaxItems");
	/* Logger::debug(string("QuadTree maxLevel: ") + std::to_string(maxLevel) +
				                ", maxItems: "  + std::to_string(maxItems)); */
	finder = new QuadTree(border, maxLevel, maxItems);
	for (auto cell : cells) {
		if (cell->getType() == PLAYER) continue;
		finder->insert(cell);
		if (!border.fullyIntersects(cell->range))
			removeCell(cell);
	}
}

void World::addCell(Cell* cell) {
	cell->exist = true;
	cell->range = { cell->getX(), cell->getY(), cell->getSize(), cell->getSize() };
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
	while (iter != boostingCells.cend()) {
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
		cell->getX(),
		cell->getY(),
		cell->getSize(),
		cell->getSize()
	};
	finder->update(cell);
}

void World::removeCell(Cell* cell) {
	auto iter = cells.begin();
	while (iter != cells.cend()) {
		if (*iter == cell) {
			cells.erase(iter);
			cell->deadTick = handle->tick;
			gcTruck.push_back(cell);
			handle->gamemode->onCellRemove(cell);
			cell->onRemoved();
			finder->remove(cell);
			setCellAsNotBoosting(cell);
			break;
		}
		iter++;
	}
}

void World::clearTruck() {
	auto iter = gcTruck.begin();
	while (iter != gcTruck.end()) {
		auto cell = *iter;
		if (handle->tick - cell->deadTick > 10) {
			delete cell;
			iter = gcTruck.erase(iter);
		} else break;
	}
}

void World::addPlayer(Player* player) {
	players.push_back(player);
	player->world = this;
	player->hasWorld = true;

	if (player->router->type == RouterType::PLAYER)
		worldChat->add((Connection*) player->router);

	handle->gamemode->onPlayerJoinWorld(player, this);
	player->router->onWorldSet();
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
	while (iter != cend) {
		if (*iter == player) {
			players.erase(iter);
			break;
		}
		iter++;
	}
	handle->gamemode->onPlayerLeaveWorld(player, this);
	player->world = nullptr;
	player->hasWorld = false;

	if (player->router && player->router->type == RouterType::PLAYER)
		worldChat->remove((Connection*) player->router);

	while (player->ownedCells.size())
		removeCell(player->ownedCells.front());
	player->router->onWorldReset();
	Logger::debug(string("player ") + to_string(player->id) + " has been removed from world " + to_string(id));
};

Point World::getRandomPos(float cellSize) {
	return {
		(float) (border.getX() - border.w + cellSize + randomZeroToOne * (2 * border.w - cellSize)),
		(float) (border.getY() - border.h + cellSize + randomZeroToOne * (2 * border.h - cellSize))
	};
}

bool World::isSafeSpawnPos(Rect& range) {
	return !finder->containAny(range, [](auto item) { return ((Cell*) item)->shouldAvoidWhenSpawning(); });
}

Point World::getSafeSpawnPos(float cellSize) {
	int tries = handle->runtime.worldSafeSpawnTries;
	while (--tries >= 0) {
		auto pos = getRandomPos(cellSize);
		Rect rect(pos.getX(), pos.getY(), cellSize, cellSize);
		if (isSafeSpawnPos(rect))
			return Point(pos);
	}
	return Point(getRandomPos(cellSize));
}

SpawnResult World::getPlayerSpawn(float cellSize) {
	double rnd = randomZeroToOne;
	float chance = handle->runtime.worldSafeSpawnFromEjectedChance;
	
	if ((chance > rnd) && (ejectedCells.size() > 0)) {
		int tries = handle->runtime.worldSafeSpawnTries;
		while (--tries >= 0) {
			std::random_device random_device;
			std::mt19937 engine{ random_device() };
			std::uniform_int_distribution<int> dist(0, ejectedCells.size() - 1);
			auto cell = *std::next(ejectedCells.begin(), dist(engine));
			Rect rect(cell->getX(), cell->getY(), cellSize, cellSize);
			if (isSafeSpawnPos(rect)) {
				removeCell(cell);
				return { cell->getColor(), { cell->getX(), cell->getX() } };
			}
		}
	}
	return { (unsigned int)-1, getSafeSpawnPos(cellSize) };
}

void World::spawnPlayer(Player* player, Point& pos, float size) {
	auto playerCell = new PlayerCell(player, pos.getX(), pos.getY(), size);
	addCell(playerCell);
	player->updateState(PlayerState::ALIVE);
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

	/*
	Logger::info(string("Pellet: ") + std::to_string(pelletCount));
	Logger::info(string("Virus: ") +  std::to_string(virusCount));
	Logger::info(string("MCell: ") +  std::to_string(motherCellCount));
	Logger::info(string("Boost: ") +  std::to_string(boostingCells.size()));
	Logger::info(string("PCell: ") +  std::to_string(playerCells.size())); */

	while (pelletCount < handle->runtime.pelletCount) {
		auto pos = getSafeSpawnPos(handle->runtime.pelletMinSize);
		addCell(new Pellet(this, this, pos.getX(), pos.getY()));
	}

	while (virusCount < handle->runtime.virusMinCount) {
		auto pos = getSafeSpawnPos(handle->runtime.virusSize);
		addCell(new Virus(this, pos.getX(), pos.getY()));
	}

	while (motherCellCount < handle->runtime.mothercellCount) {
		auto pos = getSafeSpawnPos(handle->runtime.mothercellSize);
		addCell(new MotherCell(this, pos.getX(), pos.getY()));
	}
	
	auto c_iter = boostingCells.begin();
	auto c_cend = boostingCells.cend();
	while (c_iter != c_cend) {
		if (!boostCell(*c_iter)) {
			c_iter = boostingCells.erase(c_iter);
		} else c_iter++;
	}

	for (auto c : boostingCells) {
		if (c->getType() != VIRUS && c->getType() != EJECTED_CELL) continue;
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
					eat->push_back(other);
					eat->push_back(c);
					break;
				case EatResult::EATINVD:
					eat->push_back(c);
					eat->push_back(other);
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

	auto p_iter = players.begin();
	while (p_iter != players.cend()) {

		auto player = *p_iter;
		p_iter++;

		player->checkExistence();
		if (!player->exists)
			continue;

		if (player->state == PlayerState::SPEC && !largestPlayer)
			player->updateState(PlayerState::ROAM);

		auto router = player->router;
		if (!router) continue;

		for (int j = 0, k = handle->runtime.playerSplitCap; (j < k) && (router->splitAttempts) > 0; j++) {
			router->attemptSplit();
			router->splitAttempts--;
		}

		auto nextEjectTick = handle->tick - handle->runtime.playerEjectDelay;
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
			router->onSpectateRequest();
			router->requestingSpectate = false;
		}

		if (router->spawningName.length()) {
			router->onSpawnRequest();
			router->spawningName = "";
		}

		player->updateViewArea();
	}

	compileStatistics();
	handle->gamemode->compileLeaderboard(this);

	if (stats.external <= 0) {
		if (handle->worlds.size() > handle->runtime.worldMinCount)
			toBeRemoved = true;
	}
}

void World::resolveRegidCheck(Cell* a, Cell* b) {
	float dx = b->getX() - a->getX();
	float dy = b->getY() - a->getY();
	float d = sqrt(dx * dx + dy * dy);
	float m = a->getSize() + b->getSize() - d;
	if (m <= 0) return;
	if (!d) d = 1, dx = 1, dy = 0;
	else dx /= d, dy /= d;
	float M = a->getSquareSize() + b->getSquareSize();
	float aM = b->getSquareSize() / M;
	float bM = b->getSquareSize() / M;
	a->setX(a->getX() - dx * m * aM);
	a->setY(a->getY() - dy * m * aM);
	b->setX(b->getX() + dx * m * bM);
	b->setY(b->getY() + dy * m * bM);
	bounceCell(a);
	bounceCell(b);
	updateCell(a);
	updateCell(b);
}

void World::resolveEatCheck(Cell* a, Cell* b) {
	if (!a->exist || !b->exist) return;
	float dx = b->getX() - a->getX();
	float dy = b->getY() - a->getY();
	float d = sqrt(dx * dx + dy * dy);
	if (d > a->getSize() - b->getSize() / handle->runtime.worldEatOverlapDiv) return;
	if (!handle->gamemode->canEat(a, b)) return;
	a->whenAte(b);
	b->whenEatenBy(a);
	removeCell(b);
	updateCell(a);
}

bool World::boostCell(Cell* cell) {
	float d = cell->boost.d / 9 * handle->stepMult;
	cell->setX(cell->getX() + cell->boost.dx * d);
	cell->setY(cell->getY() + cell->boost.dy * d);
	bounceCell(cell, true);
	updateCell(cell);
	if ((cell->boost.d -= d) >= 1) return true;
	cell->isBoosting = false;
	return false;
}

void World::bounceCell(Cell* cell, bool bounce) {
	float r = cell->getSize() / 2.0;
	if (cell->getX() <= border.getX() - border.w + r) {
		cell->setX(border.getX() - border.w + r);
		if (bounce) cell->boost.dx = -cell->boost.dx;
	}
	if (cell->getX() >= border.getX() + border.w - r) {
		cell->setX(border.getX() + border.w - r);
		if (bounce) cell->boost.dx = -cell->boost.dx;
	}
	if (cell->getY() <= border.getY() - border.h + r) {
		cell->setY(border.getY() - border.h + r);
		if (bounce) cell->boost.dy = -cell->boost.dy;
	}
	if (cell->getY() >= border.getY() + border.h - r) {
		cell->setY(border.getY() + border.h - r);
		if (bounce) cell->boost.dy = -cell->boost.dy;
	}
}

void World::splitVirus(Virus* virus) {
	auto newVirus = new Virus(this, virus->getX(), virus->getY());
	newVirus->boost.dx = sin(virus->splitAngle);
	newVirus->boost.dy = cos(virus->splitAngle);
	newVirus->boost.d = handle->runtime.virusSplitBoost;
	addCell(newVirus);
	setCellAsBoosting(newVirus);
}

void World::movePlayerCell(PlayerCell* cell) {
	auto router = cell->owner->router;
	if (!router || router->disconnected) return;
	float dx = router->mouseX - cell->getX();
	float dy = router->mouseY - cell->getY();
	float d = sqrt(dx * dx + dy * dy);
	if (d < 1) return;
	dx /= d; dy /= d;
	float m = std::min(cell->getMoveSpeed(), d) * handle->stepMult;
	cell->setX(cell->getX() + dx * m);
	cell->setY(cell->getY() + dy * m);
}

void World::decayPlayerCell(PlayerCell* cell) {
	float newSize = cell->getSize() - cell->getSize() * handle->gamemode->getDecayMult(cell) / 50 * handle->stepMult;
	float minSize = handle->runtime.playerMinSize;
	cell->setSize(std::max(newSize, minSize));
}

void World::launchPlayerCell(PlayerCell* cell, float size, Boost& boost) {
	cell->setSquareSize(cell->getSquareSize() - size * size);
	float x = cell->getX() + handle->runtime.playerSplitDistance * boost.dx;
	float y = cell->getY() + handle->runtime.playerSplitDistance * boost.dy;
	auto newCell = new PlayerCell(cell->owner, x, y, size);
	newCell->boost = boost;
	addCell(newCell);
	setCellAsBoosting(newCell);
}

void World::autosplitPlayerCell(PlayerCell* cell) {
	float minSplit = handle->runtime.playerMaxSize * handle->runtime.playerMaxSize;
	int cellsLeft = 1 + handle->runtime.playerMaxCells - cell->owner->ownedCells.size();
	float size = cell->getSquareSize();
	int overflow = ceil(size / minSplit);
	if (overflow == 1 || cellsLeft <= 0) return;
	float splitTimes = std::min(overflow, cellsLeft);
	float splitSize = std::min(sqrt(size / splitTimes), handle->runtime.playerMaxSize);
	for (int i = 0; i < splitTimes; i++) {
		auto angle = randomZeroToOne * 2 * PI;
		Boost boost { sin(angle), cos(angle), handle->runtime.playerSplitBoost };
		launchPlayerCell(cell, splitSize, boost);
	}
	cell->setSize(splitSize);
}

void World::splitPlayer(Player* player) {
	if (player->ownedCells.size() >= handle->runtime.playerMaxCells) return;

	auto router = player->router;
	for (auto cell : player->ownedCells) {
		if (cell->getSize() < handle->runtime.playerMinSplitSize) continue;
		float dx = router->mouseX - cell->getX();
		float dy = router->mouseY - cell->getY();
		float d = sqrt(dx * dx + dy * dy);
		if (d < 1) dx = 1, dy = 0, d = 1;
		else dx /= d, dy /= d;
		Boost boost {dx, dy, handle->runtime.playerSplitBoost };
		launchPlayerCell(cell, cell->getSize() / handle->runtime.playerSplitSizeDiv, boost);
	}
}

void World::ejectFromPlayer(Player* player) {
	float dispersion = handle->runtime.ejectDispersion;
	float loss = handle->runtime.ejectingLoss * handle->runtime.ejectingLoss;
	auto router = player->router;
	for (auto cell : player->ownedCells) {
		if (cell->getSize() < handle->runtime.playerMinEjectSize) continue;
		float dx = router->mouseX - cell->getX();
		float dy = router->mouseY - cell->getX();
		float d = sqrt(dx * dx + dy * dy);
		if (d < 1) dx = 1, dy = 0, d = 1;
		else dx /= d, dy /= d;
		float sx = cell->getX() + dx * cell->getSize();
		float sy = cell->getX() + dy * cell->getSize();
		auto newCell = new EjectedCell(this, player, sx, sy, cell->getColor());
		float a = atan2(dx, dy) - dispersion + randomZeroToOne * 2 * dispersion;
		newCell->boost.dx = sin(a);
		newCell->boost.dy = cos(a);
		newCell->boost.d = handle->runtime.ejectedCellBoost;
		addCell(newCell);
		setCellAsBoosting(newCell);
		cell->setSquareSize(cell->getSquareSize() - loss);
		updateCell(cell);
	}
}

void World::popPlayerCell(PlayerCell* cell) {
	vector<float> dist;
	distributeCellMass(cell, dist);
	for (auto mass : dist) {
		float angle = randomZeroToOne * 2 * PI;
		Boost boost { sin(angle), cos(angle), handle->runtime.playerSplitBoost };
		launchPlayerCell(cell, sqrt(mass * 100), boost);
	}
}

void World::distributeCellMass(PlayerCell* cell, std::vector<float>& dist) {
	auto player = cell->owner;
	float cellsLeft = handle->runtime.playerMaxCells - player->ownedCells.size();
	if (cellsLeft <= 0) return;
	float splitMin = handle->runtime.playerMinSplitSize;
	splitMin = splitMin * splitMin / 100;
	float cellMass = cell->getMass();
	if (handle->runtime.virusMonotonePops) {
		float amount = std::min(floor(cellMass / splitMin), cellsLeft);
		float perPiece = cellMass / (amount + 1.0);
		while (--amount >= 0) dist.push_back(perPiece);
		return;
	}
	if (cellMass / cellsLeft < splitMin) {
		float amount = 2.0, perPiece = 0;
		while ((perPiece = cellMass / (amount + 1.0)) >= splitMin && amount * 2 <= cellsLeft)
			amount *= 2.0;
		while (--amount >= 0) dist.push_back(perPiece);
		return;
	}
	float nextMass = cellMass / 2.0;
	float massLeft = cellMass / 2.0;
	while (cellsLeft > 0) {
		if (nextMass / cellsLeft < splitMin) break;
		while (nextMass >= massLeft && cellsLeft > 1)
			nextMass /= 2.0;
		dist.push_back(nextMass);
		massLeft -= nextMass;
		cellsLeft--;
	}
	nextMass = massLeft / cellsLeft;
	while (--cellsLeft >= 0) dist.push_back(nextMass);
}

void World::compileStatistics() {
	unsigned short internal = 0, external = 0, playing = 0, spectating = 0;
	for (auto p : players) {
		if (!p->router) continue;
		if (!p->router->isExternal()) { internal++; continue; }
		external++;
		if (p->state == PlayerState::ALIVE) playing++;
		else if (p->state == PlayerState::SPEC || p->state == PlayerState::ROAM)
			spectating++;
	}
	stats.limit = handle->runtime.listenerMaxConnections - handle->listener.connections.size() + external;
	stats.internal = internal;
	stats.external = external;
	stats.playing = playing;
	stats.spectating = spectating;
	stats.name = handle->runtime.serverName;
	stats.gamemode = handle->gamemode->getName();
	stats.loadTime = handle->averageTickTime / handle->stepMult;
	stats.uptime = duration_cast<seconds>(system_clock::now() - handle->startTime).count();
}

