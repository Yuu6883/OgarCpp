#include "World.h"
#include "../sockets/Router.h"
#include "../ServerHandle.h"
#include "../cells/Cell.h"
#include "../bots/PlayerBot.h"
#include <random>
#include <thread>
#include <mutex>

using std::to_string;
using std::thread;

World::World(ServerHandle* handle, unsigned int id) : handle(handle), id(id) {
	worldChat = new ChatChannel(&handle->listener);
	physicsPool = new ThreadPool(handle->getSettingInt("physicsThreads"));

	Logger::info(string("Using ") + std::to_string(handle->getSettingInt("physicsThreads")) + " threads to accelerate physics");
	Logger::info(string("Using ") + std::to_string(handle->getSettingInt("socketsThreads")) + " threads to accelerate sockets");

	float x = handle->getSettingInt("worldMapX");
	float y = handle->getSettingInt("worldMapY");
	float w = handle->getSettingInt("worldMapW");
	float h = handle->getSettingInt("worldMapH");
	Rect rect(x, y, w, h);
	setBorder(rect);
	handle->bench = true;
}

void World::afterCreation() {
	int bots = handle->getSettingInt("worldPlayerBotsPerWorld");
	if (!bots) return;
	while (bots-- > 0) {
		handle->ticker.timeout(bots * 10, [this, bots] {
			auto bot = new PlayerBot(this);
			bot->createPlayer();
			addPlayer(bot->player);
		});
	}
}

World::~World() {
	Logger::debug(string("Deallocating world (id: ") + std::to_string(id) + ")");
	for (auto player : players) {
		// Should not delete player since ServerHandle handles it
		player->hasWorld = false;
		player->world = nullptr;
	}
	players.clear();
	for (auto c : cells) delete c;
	cells.clear();
	for (auto c : gcTruck) delete c;
	gcTruck.clear();
	delete worldChat;
	delete finder;
	delete physicsPool;
}

void World::setBorder(Rect& rect) {
	border = rect;
	if (finder) delete finder;
	int maxLevel = handle->getSettingInt("worldFinderMaxLevel");
	int maxItems = handle->getSettingInt("worldFinderMaxItems");
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

void World::restart() {
	if (!shouldRestart) return;

	Logger::info(string("World (id: ") + to_string(id) + ") restarting!");
	playerCells.clear();
	boostingCells.clear();
	ejectedCells.clear();
	for (auto c : cells) delete c;
	cells.clear();
	for (auto c : gcTruck) delete c;
	gcTruck.clear();
	for (auto p : players) {
		p->lastVisibleCellData.clear();
		p->lastVisibleCells.clear();
		p->visibleCellData.clear();
		p->visibleCells.clear();
		p->ownedCellData.clear();
		p->ownedCells.clear();
		p->updateState(PlayerState::DEAD);
	}

	virusCount = 0;
	pelletCount = 0;
	motherCellCount = 0;

	handle->gamemode->compileLeaderboard(this);
	setBorder(border);
	_nextCellId = 1;
	worldChat->broadcast(nullptr, "Server restarted");
	shouldRestart = false;
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
	boostingCells.remove(cell);
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
	if (!cell->exist) return;
	cell->exist = false;
	cell->deadTick = handle->tick;
	gcTruck.push_back(cell);
	handle->gamemode->onCellRemove(cell);
	cell->onRemoved();
	finder->remove(cell);
	setCellAsNotBoosting(cell);
}

void World::clearTruck() {
	auto iter = gcTruck.begin();
	while (iter != gcTruck.end()) {
		auto cell = *iter;
		if (handle->tick - cell->deadTick > 100) {
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
		worldChat->add((Connection*)(player->router));

	handle->gamemode->onPlayerJoinWorld(player, this);
	player->router->onWorldSet();

	if (player->router->type == RouterType::PLAYER)
		Logger::debug(string("Player ") + to_string(player->id) + string(" has been added to world ") + to_string(id));
	if (!player->router->isExternal()) return;
	int minionsPerPlayer = handle->getSettingInt("worldMinionsPerPlayer");
	while (minionsPerPlayer-- > 0) {
		// TODO: add minions
	}
}

void World::removePlayer(Player* player) {
	handle->gamemode->onPlayerLeaveWorld(player, this);
	player->world = nullptr;
	player->hasWorld = false;

	if (player->router->type == RouterType::PLAYER)
		worldChat->remove((Connection*) player->router);

	while (player->ownedCells.size())
		removeCell(player->ownedCells.front());
	player->router->onWorldReset();
	Logger::debug(string("player ") + to_string(player->id) + " has been removed from world " + to_string(id));
};

void World::killPlayer(Player* player, bool instantKill) {

	if (player->state != PlayerState::ALIVE) return;
	/* if (player->router->type == RouterType::PLAYER)
		Logger::debug(string("Killing player ") + player->leaderboardName); */
	
	for (auto c : player->ownedCells) {
		if (instantKill) {
			removeCell(c);
		} else {
			c->owner = nullptr;
			c->posChanged = true;
			c->id = getNextCellId();
			if (c->data) c->data->dead = true;
		}
	}

	player->ownedCells.clear();
	player->lastVisibleCells.clear();
	player->visibleCells.clear();
	player->lastVisibleCellData.clear();
	player->visibleCellData.clear();

	for (auto c : player->ownedCellData)
		delete c;
	player->ownedCellData.clear();
}

Point World::getRandomPos(float cellSize) {
	return {
		border.getX() - border.w + cellSize + (float) randomZeroToOne * (2 * border.w - cellSize),
		border.getY() - border.h + cellSize + (float) randomZeroToOne * (2 * border.h - cellSize)
	};
}

bool World::isSafeSpawnPos(Rect& range) {
	return !finder->containAny(range, [](auto item) { return ((Cell*) item)->shouldAvoidWhenSpawning(); });
}

Point World::getSafeSpawnPos(float& cellSize, bool& failed) {
	int tries = handle->runtime.worldSafeSpawnTries;
	cellSize *= 1.2f;
	while (--tries >= 0) {
		auto pos = getRandomPos(cellSize);
		Rect rect(pos.getX(), pos.getY(), cellSize, cellSize);
		if (isSafeSpawnPos(rect)) {
			cellSize /= 1.2f;
			return Point(pos);
		}
		cellSize *= 0.998;
	}
	failed = true;
	return Point(getRandomPos(cellSize));
}

SpawnResult World::getPlayerSpawn(float& cellSize, bool& failed) {
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
	return { 0, getSafeSpawnPos(cellSize, failed) };
}

void World::spawnPlayer(Player* player, Point& pos, float size) {

	if (player->router->type == RouterType::PLAYER)
		((Connection*)player->router)->protocol->onPlayerSpawned(player);
		
	for (auto other : players) {
		if (other == player) continue;
		if (player->router->type == RouterType::PLAYER)
			((Connection*)player->router)->protocol->onPlayerSpawned(other);
		if (other->router->type == RouterType::PLAYER)
			((Connection*)other->router)->protocol->onPlayerSpawned(player);
	}

	auto playerCell = new PlayerCell(this, player, pos.getX(), pos.getY(), size);
	addCell(playerCell);
	player->joinTick = handle->tick;
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

	if (stats.loadTime > 80.0f) {
		Logger::warn(string("Server can't keep up! Load: ") + std::to_string(stats.loadTime) + "%");
		handle->bench = true;
	}

	handle->gamemode->onWorldTick(this);

	Stopwatch bench;
	bench.begin();

	for (auto c : cells)
		c->onTick();

	if (handle->bench)
		printf("\nonTickTime: %2.5fms\n", bench.lap());

	unsigned int diff = handle->runtime.pelletCount - pelletCount;
	bool failed = false;
	while (diff-- > 0) {
		float spawnSize = handle->runtime.pelletMinSize;
		auto pos = getSafeSpawnPos(spawnSize, failed);
		if (!failed) addCell(new Pellet(this, this, pos.getX(), pos.getY()));
	}

	diff = handle->runtime.virusMinCount - virusCount;
	while (diff-- > 0) {
		float spawnSize = handle->runtime.virusSize + 200.0f;
		auto pos = getSafeSpawnPos(spawnSize, failed);
		if (!failed) addCell(new Virus(this, pos.getX(), pos.getY()));
	}

	diff = handle->runtime.mothercellCount - motherCellCount;
	while (diff-- > 0) {
		float spawnSize = handle->runtime.mothercellSize + 200.0f;
		auto pos = getSafeSpawnPos(spawnSize, failed);
		if (!failed) addCell(new MotherCell(this, pos.getX(), pos.getY()));
	}

	if (handle->bench)
		printf("newCellTime: %2.5fms\n", bench.lap());
	
	auto c_iter = boostingCells.begin();
	auto c_cend = boostingCells.cend();
	while (c_iter != c_cend) {
		if (!boostCell(*c_iter)) {
			c_iter = boostingCells.erase(c_iter);
		} else c_iter++;
	}

	if (handle->bench)
		printf("boostTime: %2.5fms\n", bench.lap());

	list<pair<Cell*, Cell*>> rigid;
	list<pair<Cell*, Cell*>> eat;

	std::mutex mtx;
	int batch_size = boostingCells.size() / handle->runtime.physicsThreads + 1;
	int offset = 0;

	while (offset < boostingCells.size()) {
		auto incre = std::min(batch_size, (int) boostingCells.size() - offset);
		physicsPool->enqueue([this, offset, incre, &rigid, &eat, &mtx]() {

			auto count = incre;
			list<pair<Cell*, Cell*>> thread_rigid;
			list<pair<Cell*, Cell*>> thread_eat;

			auto start = boostingCells.cbegin();
			std::advance(start, offset);

			while (count-- && start != boostingCells.cend()) {
				auto c = *start;
				std::advance(start, 1);
				if (c->getType() == VIRUS || c->getType() == EJECTED_CELL) {
					finder->search(c->range, [&c, &thread_rigid, &thread_eat](auto o) {
						auto other = (Cell*)o;
						if (c->id == other->id) return;
						switch (c->getEatResult(other)) {
                            case EatResult::COLLIDE:
                                thread_rigid.push_back(std::make_pair(c, other));
                                break;
                            case EatResult::EAT:
                                thread_eat.push_back(std::make_pair(c, other));
                                break;
                            case EatResult::EATINVD:
                                thread_eat.push_back(std::make_pair(other, c));
                                break;
                            case EatResult::NONE:
                                break;
						}
					});
				}
			}

			mtx.lock();
			rigid.splice(rigid.begin(), thread_rigid);
			eat.splice(eat.begin(), thread_eat);
			mtx.unlock();
		});
		offset += incre;
	}

	physicsPool->waitFinished();

	/*
	for (auto c : boostingCells) {
		if (c->getType() != VIRUS && c->getType() != EJECTED_CELL) continue;
		finder->search(c->range, [c, &rigid, &eat](auto o) {
			auto other = (Cell*) o;
			if (c->id == other->id) return;
			switch (c->getEatResult(other)) {
				case EatResult::COLLIDE: 
					rigid.push_back(std::make_pair(c, other));
					break;
				case EatResult::EAT:
					eat.push_back(std::make_pair(c, other));
					break;
				case EatResult::EATINVD:
					eat.push_back(std::make_pair(other, c));
					break;
			}
		});
	} */

	if (handle->bench)
		printf("boostCheckTime: %2.5fms\n", bench.lap());

	for (auto c : playerCells) {
		movePlayerCell(c);
		decayPlayerCell(c);
		autosplitPlayerCell(c);
		bounceCell(c);
		updateCell(c);
	}

	if (handle->bench)
		printf("playerCellUpdateTime: %2.5fms\n", bench.lap());
	
	batch_size = playerCells.size() / handle->runtime.physicsThreads + 1;
	offset = 0;

	playerCells.sort([](PlayerCell* a, PlayerCell* b) { return b->getSize() * a->getAge() < a->getSize() * b->getAge(); });

	while (offset < playerCells.size()) {
		auto incre = std::min(batch_size, (int) playerCells.size() - offset);
		physicsPool->enqueue([this, offset, incre, &rigid, &eat, &mtx]() {

			auto count = incre;
			list<pair<Cell*, Cell*>> thread_rigid;
			list<pair<Cell*, Cell*>> thread_eat;

			auto start = playerCells.cbegin();
			std::advance(start, offset);

			while (count-- && start != playerCells.cend()) {
				auto c = *start;
				std::advance(start, 1);
				finder->search(c->range, [&c, &thread_rigid, &thread_eat](auto o) {
					auto other = (Cell*) o;
					if (!other->exist) return;
					if (c->id == other->id) return;
					switch (c->getEatResult(other)) {
						case EatResult::COLLIDE:
							thread_rigid.push_back(std::make_pair(c, other));
							break;
						case EatResult::EAT:
							thread_eat.push_back(std::make_pair(c, other));
							break;
						case EatResult::EATINVD:
							thread_eat.push_back(std::make_pair(other, c));
							break;
                        case EatResult::NONE:
                            break;
					}
				});
			}

			mtx.lock();
			rigid.splice(rigid.begin(), thread_rigid);
			eat.splice(eat.begin(), thread_eat);
			mtx.unlock();
		});
		offset += incre;
	}

	physicsPool->waitFinished();

	// Old single threaded physics update
	/*
	for (auto c : playerCells) {
		finder->search(c->range, [c, &rigid, &eat](auto o) {
			auto other = (Cell*)o;
			if (c->id == other->id) return;
			switch (c->getEatResult(other)) {
				case EatResult::COLLIDE:
					rigid.push_back(std::make_pair(c, other));
					break;
				case EatResult::EAT:
					eat.push_back(std::make_pair(c, other));
					break;
				case EatResult::EATINVD:
					eat.push_back(std::make_pair(other, c));
					break;
			}
		});
	}
	*/

	if (handle->bench)
		printf("playerCellRigidEatUpdateTime: %2.5fms\n", bench.lap());

	for (auto rigidPair : rigid)
		resolveRigidCheck(rigidPair.first, rigidPair.second);

	if (handle->bench)
		printf("resolveRigidTime: %2.5fms\n", bench.lap());

	for (auto eatPair : eat)
		resolveEatCheck(eatPair.first, eatPair.second);

	if (handle->bench)
		printf("resolveEatTime: %2.5fms\n", bench.lap());

	auto r_iter = cells.begin();
	while (r_iter != cells.cend())
		if (!(*r_iter)->exist) r_iter = cells.erase(r_iter);
		else r_iter++;

	largestPlayer = nullptr;
	for (auto p : players)
		if (p->score > 0 && (!largestPlayer || p->score > largestPlayer->score))
			largestPlayer = p;

	unsigned int threadPlayerCount = 0;
	lockedFinder = new QuadTree(border, finder->maxLevel, finder->maxLevel, true);

	auto p_iter = players.begin();
	while (p_iter != players.cend()) {

		auto player = *p_iter;
		if (!player->exist()) {
			p_iter = players.erase(p_iter);
			continue;
		}
		p_iter++;

		if (player->state == PlayerState::SPEC && !largestPlayer)
			player->updateState(PlayerState::ROAM);

		auto router = player->router;
		if (!router) continue;

		for (int j = 0, k = handle->runtime.playerSplitCap; (j < k) && (router->splitAttempts) > 0; j++) {
			router->attemptSplit();
			router->splitAttempts--;
		}

		auto nextEjectTick = handle->tick - handle->runtime.playerEjectDelay;
		if ((router->ejectAttempts > 0 || router->ejectMacro) && nextEjectTick >= router->ejectTick) {
			router->attemptEject();
			router->ejectAttempts = 0;
			router->ejectTick = handle->tick;
		}

		if (router->isPressingQ) {
			if (!router->hasPressedQ)
				router->onQPress();
			router->hasPressedQ = true;
		} else router->hasPressedQ = false;

		if (router->requestingSpectate)
			router->onSpectateRequest();

		if (router->requestSpawning)
			router->onSpawnRequest();

		if (router->isThreaded()) {

			if (router->busy) continue;
			for (auto d : player->ownedCellData) delete d;
			player->ownedCellData.clear();
			for (auto cell : player->ownedCells)
				player->ownedCellData.push_back(cell->getData());
			player->lockedFinder = lockedFinder;
		}
		player->updateViewArea();
	}

	if (threadPlayerCount) {
		for (auto cell : cells)
			lockedFinder->insert(cell->getData(), true);
		lockedFinder->split();
	}

	if (handle->bench)
		printf("playerUpdateTime: %2.5fms\n", bench.lap());

	compileStatistics();
	handle->gamemode->compileLeaderboard(this);

	if (stats.external <= 0)
		if (handle->worlds.size() > handle->runtime.worldMinCount)
			toBeRemoved = true;

}

void World::resolveRigidCheck(Cell* a, Cell* b) {
	if (a->getAge() <= 1 || b->getAge() <= 1) return;
	float dx = b->getX() - a->getX();
	float dy = b->getY() - a->getY();
	float d = sqrt(dx * dx + dy * dy);
	float m = a->getSize() + b->getSize() - d;
	if (m <= 0) return;
	if (!d) d = 1, dx = 1, dy = 0;
	else dx /= d, dy /= d;
	float M = a->getSquareSize() + b->getSquareSize();
	float aM = b->getSquareSize() / M;
	float bM = a->getSquareSize() / M;
	a->setX(a->getX() - dx * std::min(m, a->getSize()) * aM);
	a->setY(a->getY() - dy * std::min(m, a->getSize()) * aM);
	b->setX(b->getX() + dx * std::min(m, b->getSize()) * bM);
	b->setY(b->getY() + dy * std::min(m, b->getSize()) * bM);
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
	float d = cell->boost.d / 9 * handle->stepMult; // *cell->getSize() * 0.01f;
	float modifier = 1.0f;
	if (cell->getAge() <= 5) modifier = std::max(1.0f, 1.0f + log10f(cell->getSize()) / 10.0f);
	cell->setX(cell->getX() + cell->boost.dx * d * modifier);
	cell->setY(cell->getY() + cell->boost.dy * d * modifier);
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
	if (!cell->owner) return;
	auto router = cell->owner->router;
	if (router->disconnected) return;
	float dx = router->mouseX - cell->getX();
	float dy = router->mouseY - cell->getY();
	float d = sqrt(dx * dx + dy * dy);
	if (d < 1) return;
	float modifier = 1.0f;
	if (cell->getSize() < handle->runtime.playerMinSplitSize * 5.0f &&
		cell->getAge() <= handle->runtime.playerNoCollideDelay) modifier = -1.0f;
	dx /= d; dy /= d;
	float m = std::min(cell->getMoveSpeed() * modifier, d) * handle->stepMult;
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
	auto newCell = new PlayerCell(this, cell->owner, x, y, size);
	newCell->boost = boost;
	addCell(newCell);
	setCellAsBoosting(newCell);
}

void World::autosplitPlayerCell(PlayerCell* cell) {
	if (!cell->owner) return;
	float minSplit = handle->runtime.playerMaxSize * handle->runtime.playerMaxSize;
	int cellsLeft = 1 + handle->runtime.playerMaxCells - cell->owner->ownedCells.size();
	float size = cell->getSquareSize();
	int overflow = ceil(size / minSplit);
	if (overflow == 1 || cellsLeft <= 0) return;
	float splitTimes = std::min(overflow, cellsLeft);
	float splitSize = std::min(sqrt(size / splitTimes), handle->runtime.playerMaxSize);
	for (int i = 0; i < splitTimes; i++) {
		auto angle = randomZeroToOne * 2 * PI;
		Boost boost { (float) sin(angle), (float) cos(angle), handle->runtime.playerSplitBoost };
		launchPlayerCell(cell, splitSize, boost);
	}
	cell->setSize(splitSize);
}

void World::splitPlayer(Player* player) {
	auto router = player->router;
	int index = 0;
	auto originalLength = player->ownedCells.size();
	for (auto cell : player->ownedCells) {
		if (++index > originalLength) break;
		if (player->ownedCells.size() >= handle->runtime.playerMaxCells) return;
		if (cell->getSize() < handle->runtime.playerMinSplitSize) continue;
		float dx = router->mouseX - cell->getX();
		float dy = router->mouseY - cell->getY();
		float d = sqrt(dx * dx + dy * dy);
		if (d < 1) dx = 1, dy = 0, d = 1;
		else dx /= d, dy /= d;
		Boost boost { dx, dy, handle->runtime.playerSplitBoost };
		launchPlayerCell(cell, cell->getSize() / handle->runtime.playerSplitSizeDiv, boost);
	}
}

void World::ejectFromPlayer(Player* player) {
	if (player->justPopped) {
		handle->ticker.timeout(5, [player] { player->justPopped = false; });
		return;
	};
	float dispersion = handle->runtime.ejectDispersion;
	float loss = handle->runtime.ejectingLoss * handle->runtime.ejectingLoss;
	auto router = player->router;
	for (auto cell : player->ownedCells) {
		if (cell->getAge() <= 1) continue;
		if (cell->getSize() < handle->runtime.playerMinEjectSize) continue;
		float dx = router->mouseX - cell->getX();
		float dy = router->mouseY - cell->getY();
		float d = sqrt(dx * dx + dy * dy);
		if (d < 1) dx = 1, dy = 0, d = 1;
		else dx /= d, dy /= d;
		float sx = cell->getX() + dx * cell->getSize();
		float sy = cell->getY() + dy * cell->getSize();
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
	if (cell->owner) cell->owner->justPopped = true;
}

void World::distributeCellMass(PlayerCell* cell, std::vector<float>& dist) {
	auto player = cell->owner;
	float cellsLeft = handle->runtime.playerMaxCells - (player ? player->ownedCells.size() : 0);
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
		while (--amount >= 0) 
			dist.push_back(perPiece);
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
		if (!p->router->isExternal()) { internal++; continue; }
		external++;
		if (p->state == PlayerState::ALIVE) playing++;
		else if (p->state == PlayerState::SPEC || p->state == PlayerState::ROAM)
			spectating++;
	}
	stats.limit = handle->runtime.listenerMaxConnections;
	stats.internal = internal;
	stats.external = external;
	stats.playing = playing;
	stats.spectating = spectating;

	stats.name = handle->runtime.serverName;
	stats.gamemode = handle->gamemode->getName();
	stats.loadTime = handle->averageTickTime / handle->stepMult;
	stats.uptime = duration_cast<seconds>(system_clock::now() - handle->startTime).count();
}

