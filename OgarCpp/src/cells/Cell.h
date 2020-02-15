#pragma once

#include <math.h>
#include "../primitives/QuadTree.h"

class World;
class Player;

class Cell : public QuadItem {
	unsigned int color;
public:
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
	int getType();
	bool isSpiked();
	bool isAgitated();

	bool shouldAvoidWhenSpawning();
	bool shouldUpdate() { return posChanged || sizeChanged || colorChanged || nameChanged || skinChanged; };
	unsigned long getAge();

	double getSquareSize() { return size * size; };
	void setSquareSize(double s) { size = sqrt(s); };

	double getMass() { return size * size / 100; };
	void setMass(double s) { size = sqrt(100 * s); };

	unsigned int getColor() { return color; };
	void setColor(unsigned int c) { color = c; };

	unsigned char getEatResult(Cell* other);
	void onSpawned() {};
	void onTick() { posChanged = sizeChanged = colorChanged = nameChanged = skinChanged = false; };
	void whenAte(Cell* other) { setSquareSize(getSquareSize() + other->getSquareSize()); };
	void whenEatenBy(Cell* other) { eatenBy = other; };
	void onRemoved() {};
};