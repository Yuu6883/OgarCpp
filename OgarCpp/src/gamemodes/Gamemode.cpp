#include "Gamemode.h"
#include "../ServerHandle.h"
#include "../worlds/World.h"
#include "../cells/Cell.h"

void Gamemode::onPlayerPressQ(Player* player) {
	player->updateState(PlayerState::ROAM);
}

void Gamemode::onPlayerSplit(Player* player) {
	if (!player->hasWorld) return;
	player->world->splitPlayer(player);
}

void Gamemode::onPlayerEject(Player* player) {
	if (!player->hasWorld) return;
	player->world->ejectFromPlayer(player);
}

float Gamemode::getDecayMult(Cell* cell) {
	return cell->world->handle->runtime.playerDecayMult;
}