#include "World.h"
#include "../ServerHandle.h"

World::World(ServerHandle* handle, unsigned int id) : handle(handle), id(id) {
	double x = handle->getSettingDouble("worldMapX");
	double y = handle->getSettingDouble("worldMapY");
	double w = handle->getSettingDouble("worldMapW");
	double h = handle->getSettingDouble("worldMapH");
	Rect rect(x, y, w, h);
	setBorder(rect);
}

void World::afterCreation() {

}

World::~World() {
	while (players.size())
		removePlayer(players[0]);
	while (cells.size())
		removeCell(cells[0]);
}

void World::setBorder(Rect& rect) {
	border = rect;
	if (finder) delete finder;
	int maxLevel = handle->getSettingInt("worldFinderMaxLevel");
	int maxItems = handle->getSettingInt("worldFinderMaxItems");
	finder = new QuadTree(border, maxLevel, maxItems);
}

void World::removePlayer(Player* player) {

};
