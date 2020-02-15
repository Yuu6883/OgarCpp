#pragma once
#include <string>
#include "../primitives/Rect.h"

class ServerHandle;
class Router;
class World;

enum PlayerState { DEAD, ALIVE, SPEC, FREE };

class Player {
public:
	ServerHandle* handle;
	unsigned int id;
	Router* router;
	bool exists = true;
	std::string leaderboardName = "";
	std::string cellName = "";
	std::string chatName = "Spectator";
	std::string cellSkin = "";
	unsigned int cellColor = 0x7F7F7F;
	unsigned int chatColor = 0x7F7F7F;
	signed char state = -1;
	bool hasWorld = false;
	World* world = nullptr;
	double score = 0;
	ViewArea viewArea { 0, 0, 1920 / 2, 1080 / 2, 1 };

	Player(ServerHandle* handle, unsigned int id, Router* router);

	~Player();

	void updateState(PlayerState state);
	void updateViewArea();
	void updateVisibleCells();
	void checkExistence();
};