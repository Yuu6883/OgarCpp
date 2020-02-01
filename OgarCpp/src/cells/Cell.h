#pragma once

#include <math.h>
#include "../primitives/QuadTree.h"
#include "../worlds/World.h"

class Player;
class World;
class Boost;

class Cell : public QuadItem {
	int color;
public:
	World* world;

	unsigned long id;
	unsigned int birthTick;
	bool exist = false;

	Cell* eatenBy = nullptr;
	Boost boost;
	bool isBoosting = false;

	Player* owner = nullptr;
	double size;
	string name;
	string skin;

	bool posChanged   = false;
	bool sizeChanged  = false;
	bool colorChanged = false;
	bool nameChanged  = false;
	bool skinChanged  = false;

	Cell(World* world, double x, double y, double size, int color) : 
		QuadItem(x, y), world(world), id(world->getNextCellId()), birthTick(world->handle->tick),
		size(size), color(color) {
	}

	int getType();

	bool isSpiked();
	bool isAgitated();

	bool shouldAvoidWhenSpawning();
	bool shouldUpdate() { return posChanged || sizeChanged || colorChanged || nameChanged || skinChanged; };
	long getAge() { return (world->handle->tick - birthTick) * world->handle->stepMult; };

	double getSquareSize() { return size * size; };
	void setSquareSize(double s) { size = sqrt(s); };

	double getMass() { return size * size / 100; };
	void setMass(double s) { size = sqrt(100 * s); };

	int getColor() { return size; };
	void setColor(int c) { color = c; };

	unsigned char getEatResult(Cell* other);
	void onSpawned() {};
	void onTick() { posChanged = sizeChanged = colorChanged = nameChanged = skinChanged = false; };
	void whenAte(Cell* other) { setSquareSize(getSquareSize() + other->getSquareSize()); };
	void whenEatenBy(Cell* other) { eatenBy = other; };
	void onRemoved() {};
};