#pragma once

#include <math.h>
#include <string_view>
#include "../misc/Misc.h"
#include "../primitives/QuadTree.h"

using std::string_view;

static const double PI = atan(1) * 4;

class World;
class Player;

enum class CellType : char {
	NONE = -1,
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

	bool posChanged   = false;
	bool sizeChanged  = false;
	bool colorChanged = false;
	bool nameChanged  = false;
	bool skinChanged  = false;

	Cell(World* world, double x, double y, double size, unsigned int color);
	virtual CellType getType() { return CellType::NONE; };
	virtual bool isSpiked() { return false; };
	virtual bool isAgitated() { return false; };

	virtual string_view getName() { return ""; };
	virtual string_view getSkin() { return ""; };

	virtual bool shouldAvoidWhenSpawning() { return false; };
	bool shouldUpdate() { return posChanged || sizeChanged || colorChanged || nameChanged || skinChanged; };
	unsigned long getAge();

	double getSquareSize() { return size * size; };
	void setSquareSize(double s) { size = sqrt(s); };

	double getMass() { return size * size / 100; };
	void setMass(double s) { size = sqrt(100 * s); };

	virtual EatResult getEatResult(Cell* other) { return EatResult::COLLIDE; };
	virtual void onSpawned() {};
	virtual void onTick() { posChanged = sizeChanged = colorChanged = nameChanged = skinChanged = false; };
	virtual void whenAte(Cell* other) { setSquareSize(getSquareSize() + other->getSquareSize()); };
	virtual void whenEatenBy(Cell* other) { eatenBy = other; };
	virtual void onRemoved() {};
};

struct Spawner {
	int pelletCount = 0;
};

class PlayerCell  : public Cell {
public:
	Player* owner;
	bool _canMerge = false;
	PlayerCell(Player* owner, double x, double y, double size, unsigned int color);
	double getMoveSpeed(); 
	bool canMerge() { return _canMerge; };
	virtual CellType getType() override { return CellType::PLAYER; };
	virtual bool isSpiked() override { return false; };
	virtual bool isAgitated() override { return false; };
	virtual bool shouldAvoidWhenSpawning() override { return true; };
	virtual string_view getName() override;
	virtual string_view getSkin() override;
	virtual EatResult getEatResult(Cell* other) override;
	EatResult getDefaultEatResult(Cell* other);
	virtual void onTick() override;
	virtual void onSpawned() override;
	virtual void onRemoved() override;
};

class Virus       : public Cell {
public:
	int fedTimes = 0;
	double splitAngle = 0;
	Virus(World* world, double x, double y);
	virtual CellType getType() override { return CellType::VIRUS; };
	virtual bool isSpiked() override { return true; };
	virtual bool isAgitated() override { return false; };
	virtual bool shouldAvoidWhenSpawning() override { return true; };
	virtual EatResult getEatResult(Cell* other) override; 
	EatResult getEjectedEatResult(bool isSelf);
	virtual void whenAte(Cell* cell) override;
	virtual void whenEatenBy(Cell* cell);
	virtual void onSpawned() override;
	virtual void onRemoved() override;
};

class EjectedCell : public Cell {
public:
	Player* owner;
	EjectedCell(World* world, Player* owner, double x, double y, unsigned int color);
	virtual CellType getType() override { return CellType::EJECTED_CELL; };
	virtual bool isSpiked() override { return false; };
	virtual bool isAgitated() override { return false; };
	virtual bool shouldAvoidWhenSpawning() override { return false; };
	virtual EatResult getEatResult(Cell* other) override;
	virtual void onSpawned() override;
	virtual void onRemoved() override;
};

class Pellet : public Cell {
public:
	Spawner* spawner;
	unsigned long lastGrowTick;
	Pellet(World* world, Spawner* spawner, double x, double y);
	virtual CellType getType() override { return CellType::PELLET; };
	virtual bool isSpiked() override { return false; };
	virtual bool isAgitated() override { return false; };
	virtual bool shouldAvoidWhenSpawning() override { return false; };
	virtual EatResult getEatResult(Cell* other) override { return EatResult::NONE; };
	virtual void onTick() override;
	virtual void onSpawned() override;
	virtual void onRemoved() override;
};

class MotherCell  : public Cell, Spawner {
public:
	double activePelletFromQueue  = 0.0;
	double passivePelletFromQueue = 0.0;
	MotherCell(World* world, double x, double y);
	virtual CellType getType() override { return CellType::MOTHER_CELL; };
	virtual bool isSpiked() override { return true; };
	virtual bool isAgitated() override { return false; };
	virtual bool shouldAvoidWhenSpawning() override { return true; };
	virtual EatResult getEatResult(Cell* other) override { return EatResult::NONE; };
	virtual void onTick() override;
	void spawnPellet();
	virtual void whenAte(Cell* cell) override;
	virtual void whenEatenBy(Cell* cell) override;
	virtual void onSpawned() override;
	virtual void onRemoved() override;
};
