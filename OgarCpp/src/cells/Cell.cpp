#include "Cell.h"
#include "../worlds/World.h"
#include "../ServerHandle.h"

Cell::Cell(World* world, double x, double y, double size, int color) : 
	QuadItem(x, y), world(world), id(world->getNextCellId()), birthTick(world->handle->tick),
	size(size), color(color) {};

unsigned long Cell::getAge() { return (world->handle->tick - birthTick) * world->handle->stepMult; };