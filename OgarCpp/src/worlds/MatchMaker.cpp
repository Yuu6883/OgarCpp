#include "MatchMaker.h"
#include "../ServerHandle.h"
#include "../sockets/Listener.h"
#include "../sockets/Connection.h"
#include "World.h"

void MatchMaker::broadcastQueueLength() {
	if (!handle->runtime.matchmakerNeedsQueuing) return;
	string message = std::to_string(queued.size()) + "/" + std::to_string(handle->getSettingInt("matchmakerBulkSize")) +
		" are in queue";
	for (auto conn : queued)
		handle->listener.globalChat->directMessage(nullptr, conn, message);
}

void MatchMaker::enqueue(Connection* connection) {
	if (!handle->runtime.matchmakerNeedsQueuing)
		handle->listener.globalChat->directMessage(nullptr, connection, "joined the queue");
	queued.push_front(connection);
	broadcastQueueLength();
}

void MatchMaker::dequeue(Connection* connection) {
	if (!handle->runtime.matchmakerNeedsQueuing)
		handle->listener.globalChat->directMessage(nullptr, connection, "left the queue");
	auto iter = queued.begin();
	auto cend = queued.cend();
	while (iter != cend) {
		if (*iter == connection) {
			queued.erase(iter);
			break;
		}
		iter++;
	}
	broadcastQueueLength();
}

void MatchMaker::update() {
	int bulkSize = handle->runtime.matchmakerBulkSize;
	while (true) {
		if (queued.size() < bulkSize) return;
		auto world = getSuitableWorld();
		if (!world) return;
		for (int i = 0; i < bulkSize; i++) {
			auto dequeued = queued.back();
			queued.pop_back();
			if (handle->runtime.matchmakerNeedsQueuing)
				handle->listener.globalChat->directMessage(nullptr, dequeued, "match found!");
			world->addPlayer(dequeued->player);
		}
	}
}

World* MatchMaker::getSuitableWorld() {
	World* bestWorld = nullptr;
	for (auto pair : handle->worlds) {
		auto world = pair.second;
		if (!handle->gamemode->canJoinWorld(world)) continue;
		if (world->stats.external >= handle->runtime.worldMaxPlayers) continue;
		if (!bestWorld || world->stats.external < bestWorld->stats.external)
			bestWorld = world;
	}
	if (bestWorld) return bestWorld;
	else if (handle->worlds.size() < handle->runtime.worldMaxCount)
		return handle->createWorld();
	return nullptr;
}