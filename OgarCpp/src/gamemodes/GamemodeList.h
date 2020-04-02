#pragma once

#include <map>
#include "Gamemode.h"

using std::map;

class GamemodeList {
public:
	ServerHandle* handle = nullptr;
	map<string, Gamemode*> store;
	GamemodeList(ServerHandle* handle) : handle(handle) {};
	void registerGamemode(Gamemode* gamemode);
	void setGamemode(string name);
};