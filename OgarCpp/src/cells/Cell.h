#pragma once

#include <math.h>
#include "../primitives/QuadTree.h"

class World;
class Player;

enum CellType : unsigned char {
	PLAYER,
	PELLET,
	VIRUS,
	EJECTED_CELL,
	MOTHER_CELL
};

enum EatResult : unsigned char {
	COLLIDE = 1,
	EAT,
	EATEN
};

class Cell : public QuadItem {
public:
	unsigned int color;
	World* world;

	unsigned long id;
	unsigned long birthTick;
	bool exist = false;

	Cell* eatenBy = nullptr;
	Boost boost;
	bool isBoosting = false;

	Player* owner = nullptr;
	double size;
	std::string name;
	std::string skin;

	bool posChanged   = false;
	bool sizeChanged  = false;
	bool colorChanged = false;
	bool nameChanged  = false;
	bool skinChanged  = false;

	Cell(World* world, double x, double y, double size, int color);
	virtual CellType getType();
	bool isSpiked();
	bool isAgitated();

	virtual bool shouldAvoidWhenSpawning();
	bool shouldUpdate() { return posChanged || sizeChanged || colorChanged || nameChanged || skinChanged; };
	unsigned long getAge();

	double getSquareSize() { return size * size; };
	void setSquareSize(double s) { size = sqrt(s); };

	double getMass() { return size * size / 100; };
	void setMass(double s) { size = sqrt(100 * s); };

	unsigned int getColor() { return color; };
	void setColor(unsigned int c) { color = c; };

	EatResult getEatResult(Cell* other);
	void onSpawned() {};
	void onTick() { posChanged = sizeChanged = colorChanged = nameChanged = skinChanged = false; };
	void whenAte(Cell* other) { setSquareSize(getSquareSize() + other->getSquareSize()); };
	void whenEatenBy(Cell* other) { eatenBy = other; };
	void onRemoved() {};
};

class PlayerCell  : public Cell {};
class Virus       : public Cell {};
class EjectedCell : public Cell {};
class MotherCell  : public Cell {};