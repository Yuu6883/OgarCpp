#include "Cell.h"
#include "../worlds/World.h"
#include "../ServerHandle.h"

Cell::Cell(World* world, double x, double y, double size, unsigned int color) : 
	QuadItem(x, y), world(world), id(world->getNextCellId()), birthTick(world->handle->tick),
	size(size), color(color) {};

unsigned long Cell::getAge() { return (world->handle->tick - birthTick) * world->handle->stepMult; };

PlayerCell::PlayerCell(Player* owner, double x, double y, double size):
	Cell(owner->world, x, y, size, owner->cellColor), owner(owner) {};

string_view PlayerCell::getName() { return owner->cellName; };
string_view PlayerCell::getSkin() { return owner->cellSkin; };

double PlayerCell::getMoveSpeed() {
	return 88 * pow(size, -0.4396754) * owner->handle->runtime.playerMoveMult;
}

EatResult PlayerCell::getEatResult(Cell* other) {
	if (other->getType() == CellType::PLAYER) {
		auto delay = world->handle->runtime.playerNoCollideDelay;
		if (((PlayerCell*)other)->id == id) {
			if (other->getAge() < delay || getAge() < delay) return EatResult::NONE;
			if (canMerge() && ((PlayerCell*)other)->canMerge()) return EatResult::EAT;
			return EatResult::COLLIDE;
		}
		if (((PlayerCell*)other)->owner->team == owner->team && owner->team >= 0) {}
		return (other->getAge() < delay || getAge() < delay) ? EatResult::NONE : EatResult::COLLIDE;
		return getDefaultEatResult(other);
	}
	if (other->getType() == CellType::MOTHER_CELL &&
		other->size > size* world->handle->runtime.worldEatMult) return EatResult::EATINVD;
	if (other->getType() == CellType::PELLET) return EatResult::EAT;
	return getDefaultEatResult(other);
}

EatResult PlayerCell::getDefaultEatResult(Cell* other) {
	return other->size * world->handle->runtime.worldEatMult > size ? EatResult::NONE : EatResult::EAT;
}

void PlayerCell::onTick() {
	Cell::onTick();
	if (color != owner->cellColor) color = owner->cellColor;
	auto delay = world->handle->runtime.playerNoMergeDelay;
	if (world->handle->runtime.playerMergeTime > 0) {
		auto initial = 25 * world->handle->runtime.playerMergeTime;
		auto increase = (int)round(25 * size * world->handle->runtime.playerMergeTimeIncrease);
		auto sumOrMax = world->handle->runtime.playerMergeNewVersion ? std::max(initial, increase) : initial + increase;
		delay = std::max(delay, sumOrMax);
	}
	_canMerge = getAge() >= delay;
}

void PlayerCell::onSpawned() {
	owner->router->onNewOwnedCell(this);
	owner->ownedCells.push_back(this);
	world->playerCells.push_back(this);
}

void PlayerCell::onRemoved() {
	auto iter = world->playerCells.rbegin();
	auto cend = world->playerCells.crend();
	while (iter != cend) {
		if (*iter == this) {
			world->playerCells.erase(iter.base());
			break;
		}
		iter++;
	}
	iter = owner->ownedCells.rbegin();
	cend = owner->ownedCells.crend();
	while (iter != cend) {
		if (*iter == this) {
			owner->ownedCells.erase(iter.base());
			break;
		}
		iter++;
	}
	owner->updateState(PlayerState::DEAD);
}

Virus::Virus(World* world, double x, double y) :
	Cell(world, x, y, world->handle->runtime.virusSize, 0x33FF33) {};

EatResult Virus::getEatResult(Cell* other) {
	if (other->getType() == CellType::EJECTED_CELL) return getEjectedEatResult(true);
	if (other->getType() == CellType::MOTHER_CELL) return EatResult::EATINVD;
	return EatResult::NONE;
}

EatResult Virus::getEjectedEatResult(bool isSelf) {
	return world->virusCount >= world->handle->runtime.virusMaxCount ?
		EatResult::NONE : isSelf ? EatResult::EAT : EatResult::EATINVD;
}

void Virus::whenAte(Cell* cell) {
	auto runtime = &world->handle->runtime;
	if (runtime->virusPushing) {
		auto d = boost.d + runtime->virusPushBoost;
		boost.dx = (boost.dx * boost.d + cell->boost.dx * runtime->virusPushBoost) / d;
		boost.dy = (boost.dy * boost.d + cell->boost.dy * runtime->virusPushBoost) / d;
		boost.d = d;
		world->setCellAsBoosting(this);
	}
	else {
		double angle = atan2(cell->boost.dx, cell->boost.dy);
		if (++fedTimes >= runtime->virusFeedTimes) {
			fedTimes = 0;
			size = runtime->virusSize;
			world->splitVirus(this);
		}
		else Cell::whenAte(cell);
	}
}

void Virus::whenEatenBy(Cell* cell) {
	Cell::whenEatenBy(cell);
	if (cell->getType() == CellType::PLAYER) 
		world->popPlayerCell((PlayerCell*)cell);
}

void Virus::onSpawned() {
	world->virusCount++;
}

void Virus::onRemoved() {
	world->virusCount--;
}

EjectedCell::EjectedCell(World* world, Player* owner, double x, double y, unsigned int color) :
	Cell(world, x, y, world->handle->runtime.ejectedSize, color), owner(owner) {};

EatResult EjectedCell::getEatResult(Cell* other) {
	if (other->getType() == CellType::VIRUS) return ((Virus*)other)->getEjectedEatResult(false);
	if (other->getType() == CellType::MOTHER_CELL) return EatResult::EATINVD;
	if (other->getType() == CellType::EJECTED_CELL) {
		if (!other->isBoosting) world->setCellAsBoosting(other);
		return EatResult::COLLIDE;
	}
	return EatResult::NONE;
}

void EjectedCell::onSpawned() {
	world->ejectedCells.push_back(this);
}

void EjectedCell::onRemoved() {
	auto iter = world->ejectedCells.rbegin();
	auto cend = world->ejectedCells.crend();
	while (iter != cend) {
		if (*iter == this) {
			world->ejectedCells.erase(iter.base());
			break;
		}
		iter++;
	}
}

Pellet::Pellet(World* world, Spawner* spawner, double x, double y):
	Cell(world, x, y, world->handle->runtime.pelletMinSize, randomColor()),
	spawner(spawner), lastGrowTick(birthTick) {};

void Pellet::onTick() {
	Cell::onTick();
	if (size >= world->handle->runtime.pelletMaxSize) return;
	if (world->handle->tick - lastGrowTick > world->handle->runtime.pelletGrowTicks / world->handle->stepMult) {
		lastGrowTick = world->handle->tick;
		setMass(getMass() + 1);
	}
}

void Pellet::onSpawned() {
	world->pelletCount++;
}

void Pellet::onRemoved() {
	world->pelletCount--;
}

MotherCell::MotherCell(World* world, double x, double y) :
	Cell(world, x, y, world->handle->runtime.mothercellSize, 0xCE6363) {};

void MotherCell::onTick() {
	auto runtime = &world->handle->runtime;
	auto mcSize = runtime->mothercellSize;
	auto pSize = runtime->pelletMinSize;
	auto minSpawnSqSize = mcSize * mcSize + pSize * pSize;

	activePelletFromQueue += runtime->mothercellActiveSpawnSpeed * world->handle->stepMult;
	passivePelletFromQueue += ((double)rand() / (RAND_MAX)) * runtime->mothercellPassiveSpawnChance * world->handle->stepMult;

	while (activePelletFromQueue > 0) {
		if (getSquareSize() > minSpawnSqSize)
			spawnPellet(), setSquareSize(getSquareSize() - pSize);
		else if (size > mcSize)
			size = mcSize;
		activePelletFromQueue--;
	}

	while (passivePelletFromQueue > 0) {
		if (pelletCount < runtime->mothercellMaxPellets)
			spawnPellet();
		passivePelletFromQueue--;
	}
}

void MotherCell::spawnPellet() {
	auto angle = ((double)rand() / (RAND_MAX)) * 2 * PI;
	auto x = this->x + size * sin(angle);
	auto y = this->y + size * cos(angle);
	auto pellet = new Pellet(world, this, x, y);
	pellet->boost.dx = sin(angle);
	pellet->boost.dy = cos(angle);
	auto d = world->handle->runtime.mothercellPelletBoost;
	pellet->boost.d = d / 2.0 + ((double)rand() / (RAND_MAX)) * d / 2.0;
	world->addCell(pellet);
	world->setCellAsBoosting(pellet);
}

void MotherCell::whenAte(Cell* cell) {
	Cell::whenAte(cell);
	size = std::min(size, world->handle->runtime.mothercellMaxSize);
}

void MotherCell::whenEatenBy(Cell* cell) {
	Cell::whenEatenBy(cell);
	if (cell->getType() == CellType::PLAYER) world->popPlayerCell((PlayerCell*)cell);
}

void MotherCell::onSpawned() {
	world->motherCellCount++;
}

void MotherCell::onRemoved() {
	world->motherCellCount--;
}

