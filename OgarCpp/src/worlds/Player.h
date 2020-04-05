#pragma once
#include <string>
#include <map>
#include <list>
#include "../primitives/Rect.h"
#include "../cells/Cell.h"

using std::string;
using std::map;
using std::list;

class ServerHandle;
class PlayerCell;
class Router;
class World;

enum class PlayerState : unsigned char { 
	DEAD, ALIVE, SPEC, ROAM 
};

class Player {
public:
	ServerHandle* handle;
	unsigned int id;
	Router* router;
	string leaderboardName = "";
	string cellName = "";
	string chatName = "Spectator";
	string cellSkin = "";
	unsigned int cellColor = 0x7F7F7F;
	unsigned int chatColor = 0x7F7F7F;
	PlayerState state = PlayerState::DEAD;
	bool hasWorld = false;
	World* world = nullptr;
	short team = -1;
	float score = 0;
	list<PlayerCell*> ownedCells;
	map<unsigned int, Cell*> visibleCells;
	map<unsigned int, Cell*> lastVisibleCells;
	
	ViewArea viewArea = ViewArea(0, 0, 1920 / 2, 1080 / 2, 1);

	Player(ServerHandle* handle, unsigned int id, Router* router);

	~Player();

	void updateState(PlayerState state);
	void updateViewArea();
	void updateVisibleCells();
	bool exist();
};