#include "GamemodeList.h"
#include "../ServerHandle.h"

void GamemodeList::registerGamemode(Gamemode* gamemode) {
    if (!gamemode) return;
    if (!store.contains(gamemode->getName())) {
        store.insert(std::make_pair(gamemode->getName(), gamemode));
    }
}

void GamemodeList::setGamemode(string name) {
	if (store.contains(name)) {
		handle->gamemode = new Gamemode(*store[name]);
	}
}