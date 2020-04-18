#include <algorithm>
#include "FFA.h"
#include "../misc/Misc.h"
#include "../worlds/Player.h"
#include "../worlds/World.h"
#include "../sockets/Connection.h"
#include "../ServerHandle.h"

void FFA::onPlayerSpawnRequest(Player* player, string name, string skin) {
	if (!player->hasWorld) return;
	if (player->state == PlayerState::ALIVE && handle->runtime.respawnEnabled) {
		player->world->killPlayer(player);
	}
	float size = player->router->type == RouterType::MINION ?
		handle->runtime.minionSpawnSize : handle->runtime.playerSpawnSize;
	auto spawnResult = player->world->getPlayerSpawn(size);
	unsigned int color = spawnResult.color || randomColor();
	player->cellName = player->chatName = player->leaderboardName = name;
	player->cellSkin = skin;
	player->chatColor = player->cellColor = color;
	player->world->spawnPlayer(player, spawnResult.pos, size);
}

void FFA::compileLeaderboard(World* world) {
	leaderboard.clear();
	for (auto player : world->players)
		if (player->score > 0)
			leaderboard.push_back(player);
	std::sort(leaderboard.begin(), leaderboard.end(), [](Player* a, Player* b) {
		return b->score > a->score;
	});
}

void FFA::sendLeaderboard(Connection* connection) {
	if (!connection->hasPlayer) return;
	auto player = connection->player;
	if (!player->hasWorld) return;
	if (player->world->frozen) return;
	vector<LBEntry*> lbData;
	FFAEntry* lbSelfData = nullptr;
	int position = 1;
	for (auto player : leaderboard) {
		auto entry = new FFAEntry();
		entry->position = position++;
		entry->name = player->leaderboardName;
		entry->cellId = player->ownedCells.front()->id;
		if (connection->player == player) {
			entry->highlighted = true;
			lbSelfData = entry;
		}
	}
	connection->protocol->onLeaderboardUpdate(LBType::FFA, lbData, lbSelfData);
}