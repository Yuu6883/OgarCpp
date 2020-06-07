#include "Cell.h"
#include "../worlds/World.h"
#include "../ServerHandle.h"

Cell::Cell(World* world, float x, float y, float size, unsigned int color) : 
	QuadItem(x, y), world(world), id(world->getNextCellId()), birthTick(world->handle->tick),
	size(size), color(color) {};

unsigned long Cell::getAge() { return (world->handle->tick - birthTick) * world->handle->stepMult; };

CellData* Cell::getData() {
	data = new CellData(x, y, getType(), id, owner ? owner->id : 0, 
		getAge(), eatenBy ? eatenBy->id : 0, size, owner ? 0 : 1);
	return data;
}

PlayerCell::PlayerCell(World* world, Player* owner, float x, float y, float size):
	Cell (world, x, y, size, owner ? owner->cellColor : 0) {
	this->owner = owner;
};

string_view PlayerCell::getName() { return owner->cellName; };
string_view PlayerCell::getSkin() { return owner->cellSkin; };

float PlayerCell::getMoveSpeed() {
	return 88 * pow(size, -0.4396754) * owner->handle->runtime.playerMoveMult;
}

EatResult PlayerCell::getEatResult(Cell* other) {
	if (other->getType() == PLAYER) {
		if (!owner && !other->owner) return EatResult::COLLIDE;
		if (!owner) return EatResult::NONE;
		auto delay = world->handle->runtime.playerNoCollideDelay;
		if (owner && other->owner && other->owner->id == owner->id) {
			if (other->getAge() < delay || getAge() < delay) return EatResult::NONE;
			if (canMerge() && ((PlayerCell*)other)->canMerge()) return EatResult::EAT;
			return EatResult::COLLIDE;
		}
		if (owner && other->owner && owner->team >= 0 && other->owner->team == owner->team)
			return (other->getAge() < delay || getAge() < delay) ? EatResult::NONE : EatResult::COLLIDE;
		return getDefaultEatResult(other);
	}
	if (!owner) return EatResult::NONE;
	if (other->getType() == MOTHER_CELL &&
		other->getSize() > size* world->handle->runtime.worldEatMult) return EatResult::EATINVD;
	if (other->getType() == PELLET) return EatResult::EAT;
	return getDefaultEatResult(other);
}

EatResult PlayerCell::getDefaultEatResult(Cell* other) {
	if (world->handle->tick - owner->joinTick < world->handle->runtime.spawnProtection && other->owner != owner) return EatResult::NONE;
	return other->getSize() * world->handle->runtime.worldEatMult > size ? EatResult::NONE : EatResult::EAT;
}

void PlayerCell::onTick() {
	Cell::onTickDefault();
	if (!owner) {
		deadTick++;
		if (deadTick > world->handle->runtime.worldPlayerDisposeDelay) world->removeCell(this);
		return;
	}
	if (color != owner->cellColor) {
		color = owner->cellColor;
		colorChanged = true;
	}
	auto delay = world->handle->runtime.playerNoMergeDelay;
	if (world->handle->runtime.playerMergeTime > 0) {
		auto initial = 25 * world->handle->runtime.playerMergeTime;
		auto increase = round(25 * size * world->handle->runtime.playerMergeTimeIncrease);
		auto sumOrMax = world->handle->runtime.playerMergeNewVersion ? std::max(initial, increase) : initial + increase;
		delay = std::max(delay, sumOrMax);
	}
	_canMerge = getAge() >= delay;
}

void PlayerCell::onSpawned() {
	if (owner) {
		owner->router->onNewOwnedCell(this);
		owner->ownedCells.push_back(this);
	}
}

void PlayerCell::onRemoved() {
	if (!owner) return;
	owner->ownedCells.remove(this);
	if (!owner->ownedCells.size() && eatenBy && eatenBy->owner)
		eatenBy->owner->killCount++;

	owner->updateState(PlayerState::DEAD);
}

Virus::Virus(World* world, float x, float y) :
	Cell(world, x, y, world->handle->runtime.virusSize, 0x33FF33) {};

EatResult Virus::getEatResult(Cell* other) {
	if (other->getType() == EJECTED_CELL) return getEjectedEatResult(true);
	if (other->getType() == MOTHER_CELL) return EatResult::EATINVD;
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
	} else {
		splitAngle = atan2(cell->boost.dx, cell->boost.dy);
		if (++fedTimes >= runtime->virusFeedTimes) {
			fedTimes = 0;
			size = runtime->virusSize;
			world->splitVirus(this);
		} else Cell::whenAteDefault(cell);
	}
}

void Virus::whenEatenBy(Cell* cell) {
	Cell::whenEatenByDefault(cell);
	if (cell->getType() == PLAYER) 
		world->popPlayerCell((PlayerCell*)cell);
}

void Virus::onSpawned() {
	world->virusCount++;
}

void Virus::onRemoved() {
	world->virusCount--;
}

EjectedCell::EjectedCell(World* world, Player* owner, float x, float y, unsigned int color) :
	Cell(world, x, y, world->handle->runtime.ejectedSize, color) {
	this->owner = owner;
};

EatResult EjectedCell::getEatResult(Cell* other) {
	if (other->getType() == VIRUS) return ((Virus*)other)->getEjectedEatResult(false);
	if (other->getType() == MOTHER_CELL) return EatResult::EATINVD;
	if (other->getType() == EJECTED_CELL) return EatResult::COLLIDE;
	return EatResult::NONE;
}

void EjectedCell::onSpawned() {
}

void EjectedCell::onRemoved() {
}

Pellet::Pellet(World* world, Spawner* spawner, float x, float y):
	Cell(world, x, y, world->handle->runtime.pelletMinSize, randomColor()),
	spawner(spawner), lastGrowTick(birthTick) {};

void Pellet::onTick() {
	Cell::onTickDefault();
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

MotherCell::MotherCell(World* world, float x, float y) :
	Cell(world, x, y, world->handle->runtime.mothercellSize, 0xCE6363) {};

void MotherCell::onTick() {
	auto runtime = &world->handle->runtime;
	auto mcSize = runtime->mothercellSize;
	auto pSize = runtime->pelletMinSize;
	auto minSpawnSqSize = mcSize * mcSize + pSize * pSize;

	activePelletFromQueue += runtime->mothercellActiveSpawnSpeed * world->handle->stepMult;
	passivePelletFromQueue += ((float)rand() / (RAND_MAX)) * runtime->mothercellPassiveSpawnChance * world->handle->stepMult;

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
	auto angle = ((float)rand() / (RAND_MAX)) * 2 * PI;
	auto x = this->x + size * sin(angle);
	auto y = this->y + size * cos(angle);
	auto pellet = new Pellet(world, this, x, y);
	pellet->boost.dx = sin(angle);
	pellet->boost.dy = cos(angle);
	auto d = world->handle->runtime.mothercellPelletBoost;
	pellet->boost.d = d / 2.0 + ((double) rand() / (RAND_MAX)) * d / 2.0;
	world->addCell(pellet);
}

void MotherCell::whenAte(Cell* cell) {
	Cell::whenAteDefault(cell);
	size = std::min(size, world->handle->runtime.mothercellMaxSize);
}

void MotherCell::whenEatenBy(Cell* cell) {
	Cell::whenEatenByDefault(cell);
	if (cell->getType() == PLAYER) world->popPlayerCell((PlayerCell*)cell);
}

void MotherCell::onSpawned() {
	world->motherCellCount++;
}

void MotherCell::onRemoved() {
	world->motherCellCount--;
}

