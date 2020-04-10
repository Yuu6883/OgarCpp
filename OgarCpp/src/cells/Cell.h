#pragma once

struct Spawner {
	int pelletCount = 0;
};

#include <math.h>
#include <string_view>
#include "../misc/Misc.h"
#include "../primitives/QuadTree.h"

using std::string_view;

static const float PI = atan(1) * 4;

class World;
class Player;

enum CellType : char {
	PLAYER,
	PELLET,
	VIRUS,
	EJECTED_CELL,
	MOTHER_CELL
};

enum class EatResult : unsigned char {
	NONE,
	COLLIDE,
	EAT,
	EATINVD
};

class Cell : public QuadItem {
protected:
	unsigned int color;
	float size;
public:

	World* world;

	unsigned int id;
	unsigned long birthTick;
	unsigned long deadTick = 0;
	bool exist = true;

	Cell* eatenBy = nullptr;
	Boost boost = Boost();
	bool isBoosting = false;

	Player* owner = nullptr;

	bool posChanged   = false;
	bool sizeChanged  = false;
	bool colorChanged = false;
	bool nameChanged  = false;
	bool skinChanged  = false;

	Cell(World* world, float x, float y, float size, unsigned int color);

	void setX(float x) {
		if (x != this->x) {
			this->x = x;
			posChanged = true;
		}
	}

	void setY(float y) {
		if (y != this->y) {
			this->y = y;
			posChanged = true;
		}
	}

	float getSize() { return size; };
	void setSize(float size) {
		if (size != this->size) {
			this->size = size;
			sizeChanged = true;
		}
	}

	unsigned int getColor() {
		return color;
	}

	void setColor(unsigned int color) {
		if (color != this->color) {
			this->color = color;
			colorChanged = true;
		}
	}

	virtual CellType getType() = 0;
	virtual bool isSpiked() = 0;
	virtual bool isAgitated() = 0;

	virtual string_view getName() = 0;
	virtual string_view getSkin() = 0;

	virtual bool shouldAvoidWhenSpawning() = 0;
	bool shouldUpdate() { return posChanged || sizeChanged || colorChanged || nameChanged || skinChanged; };
	unsigned long getAge();

	float getSquareSize() { return size * size; };
	void setSquareSize(float s) { size = sqrt(s); };

	float getMass() { return size * size / 100; };
	void setMass(float s) { size = sqrt(100 * s); };

	virtual EatResult getEatResult(Cell* other) = 0;

	void onTickDefault() { posChanged = sizeChanged = colorChanged = nameChanged = skinChanged = false; };
	virtual void onTick() = 0;

	void whenAteDefault(Cell* other) { setSquareSize(getSquareSize() + other->getSquareSize()); };
	virtual void whenAte(Cell* other) = 0;

	void whenEatenByDefault(Cell* other) { eatenBy = other; };
	virtual void whenEatenBy(Cell* other) = 0;

	virtual void onSpawned() = 0;
	virtual void onRemoved() = 0;
};

class PlayerCell : public Cell {
public:
	Player* owner;
	bool _canMerge = false;
	PlayerCell(Player* owner, float x, float y, float size);
	float getMoveSpeed(); 
	bool canMerge() { return _canMerge; };
	CellType getType() { return PLAYER; };
	bool isSpiked() { return false; };
	bool isAgitated() { return false; };
	bool shouldAvoidWhenSpawning() { return true; };
	string_view getName();
	string_view getSkin();
	EatResult getEatResult(Cell* other);
	EatResult getDefaultEatResult(Cell* other);
	void whenAte(Cell* other) { Cell::whenAteDefault(other); };
	void whenEatenBy(Cell* other) { Cell::whenEatenByDefault(other); };
	void onTick();
	void onSpawned();
	void onRemoved();
};

class Virus : public Cell {
public:
	int fedTimes = 0;
	float splitAngle = 0;
	Virus(World* world, float x, float y);
	CellType getType() { return VIRUS; };
	string_view getName() { return string_view(""); };
	string_view getSkin() { return string_view(""); };
	bool isSpiked() { return true; };
	bool isAgitated() { return false; };
	bool shouldAvoidWhenSpawning() { return true; };
	EatResult getEatResult(Cell* other); 
	EatResult getEjectedEatResult(bool isSelf);
	void onTick() { Cell::onTickDefault(); };
	void whenAte(Cell* cell);
	void whenEatenBy(Cell* cell);
	void onSpawned();
	void onRemoved();
};

class EjectedCell : public Cell {
public:
	Player* owner;
	EjectedCell(World* world, Player* owner, float x, float y, unsigned int color);
	CellType getType() { return EJECTED_CELL; };
	string_view getName() { return string_view(""); };
	string_view getSkin() { return string_view(""); };
	bool isSpiked() { return false; };
	bool isAgitated() { return false; };
	bool shouldAvoidWhenSpawning() { return false; };
	EatResult getEatResult(Cell* other);
	void onTick() { Cell::onTickDefault(); };
	void whenAte(Cell* other) { Cell::whenAteDefault(other); };
	void whenEatenBy(Cell* other) { Cell::whenEatenByDefault(other); };
	void onSpawned();
	void onRemoved();
};

class Pellet : public Cell {
public:
	Spawner* spawner;
	unsigned long lastGrowTick;
	Pellet(World* world, Spawner* spawner, float x, float y);
	CellType getType() { return PELLET; };
	string_view getName() { return string_view(""); };
	string_view getSkin() { return string_view(""); };
	bool isSpiked() { return false; };
	bool isAgitated() { return false; };
	bool shouldAvoidWhenSpawning() { return false; };
	EatResult getEatResult(Cell* other) { return EatResult::NONE; };
	void onTick();
	void whenAte(Cell* other) { Cell::whenAteDefault(other); };
	void whenEatenBy(Cell* other) { Cell::whenEatenByDefault(other); };
	void onSpawned();
	void onRemoved();
};

class MotherCell  : public Cell, Spawner {
public:
	float activePelletFromQueue  = 0.0;
	float passivePelletFromQueue = 0.0;
	MotherCell(World* world, float x, float y);
	CellType getType() { return MOTHER_CELL; };
	string_view getName() { return string_view(""); };
	string_view getSkin() { return string_view(""); };
	bool isSpiked() { return true; };
	bool isAgitated() { return false; };
	bool shouldAvoidWhenSpawning() { return true; };
	EatResult getEatResult(Cell* other) { return EatResult::NONE; };
	void onTick();
	void whenAte(Cell* cell);
	void whenEatenBy(Cell* cell);
	void spawnPellet();
	void onSpawned();
	void onRemoved();
};
