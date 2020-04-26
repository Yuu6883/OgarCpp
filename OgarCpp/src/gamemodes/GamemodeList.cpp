#include "GamemodeList.h"
#include "../ServerHandle.h"

void GamemodeList::registerGamemode(Gamemode* gamemode) {
    if (!gamemode) return;
    if (store.find(gamemode->getName()) == store.cend())
        store.insert(std::make_pair(gamemode->getName(), gamemode));
}

void GamemodeList::setGamemode(string name) {
    if (store.find(name) != store.cend()) {
        handle->gamemode = store[name]->clone();
        Logger::info(string("Setting gamemode to ") + name);
    }
    else Logger::warn(string("Unknown gamemode: ") + name);
}