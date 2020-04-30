#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include "../primitives/Rect.h"
#include "../cells/Cell.h"

using std::string;
using std::unordered_map;
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
	bool justPopped = false;
	World* world = nullptr;
	short team = -1;
	float score = 0;
	unsigned short killCount = 0;
	float maxScore = 0;
	unsigned long joinTick = 0;
	
	// For sequential buffering
	list<PlayerCell*> ownedCells;
	unordered_map<unsigned int, Cell*> visibleCells;
	unordered_map<unsigned int, Cell*> lastVisibleCells;

	// For threaded buffering
	list<CellData*> ownedCellData;
	unordered_map<unsigned int, CellData*> visibleCellData;
	unordered_map<unsigned int, CellData*> lastVisibleCellData;
	QuadTree* lockedFinder = nullptr;
	
	ViewArea viewArea = ViewArea(0, 0, 1920 / 2, 1080 / 2, 1);

	Player(ServerHandle* handle, unsigned int id, Router* router);

	~Player();

	void updateState(PlayerState state);
	void updateViewArea();
	void updateVisibleCells(bool threaded = false);
	bool exist();
};