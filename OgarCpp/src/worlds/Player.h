#pragma once
#include <string>
#include <map>
#include <vector>
#include "../primitives/Rect.h"
#include "../cells/Cell.h"

using std::string;
using std::map;
using std::vector;

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
	bool exists = true;
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
	double score = 0;
	vector<PlayerCell*> ownedCells;
	map<unsigned int, Cell*> visibleCells;
	map<unsigned int, Cell*> lastVisibleCells;
	
	ViewArea viewArea { 0, 0, 1920 / 2, 1080 / 2, 1 };

	Player(ServerHandle* handle, unsigned int id, Router* router);

	~Player();

	void updateState(PlayerState state);
	void updateViewArea();
	void updateVisibleCells();
	void checkExistence();
};