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
		handle->listener.globalChat->directMessage(nullptr, connection, "You joined the queue");
	queued.push_front(connection);
	broadcastQueueLength();
}

void MatchMaker::dequeue(Connection* connection) {
	if (!handle->runtime.matchmakerNeedsQueuing)
		handle->listener.globalChat->directMessage(nullptr, connection, "You left the queue");
	queued.remove(connection);
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
				handle->listener.globalChat->directMessage(nullptr, dequeued, "Match found!");
			world->addPlayer(dequeued->player);
		}
	}
}

World* bestWorld = nullptr;
World* MatchMaker::getSuitableWorld() {
	std::for_each(handle->worlds.begin(), handle->worlds.end(), [this](auto pair) {
		World* world = pair.second;
		if (world->toBeRemoved) return;
		if (!handle->gamemode->canJoinWorld(world)) return;
		if (world->stats.external >= handle->runtime.worldMaxPlayers) return;
		if (!bestWorld || world->stats.external < bestWorld->stats.external)
			bestWorld = world;
	});

	if (bestWorld) return bestWorld;
	else if (handle->worlds.size() < handle->runtime.worldMaxCount)
		return handle->createWorld();
	return nullptr;
}