#include "Listener.h"
#include "Connection.h"
#include "../ServerHandle.h"
#include "../protocols/Protocol.h"
#include "../primitives/Logger.h"
#define MAX_FRAME_SIZE 512

string ipv4ToString(unsigned int ipv4) {
	return "";
}

void Connection::close() {
	if (!socketDisconnected) {
		closeSocket(CLOSE_GOING_AWAY, "Manual connection close call");
		return;
	}
	Router::close();
	disconnected = true;
	disconnectedTick = listener->getTick();
	listener->onDisconnection(this, closeCode, closeReason);
}

void Connection::onSocketClose(int code, string_view reason) {
	if (socketDisconnected) return;
	Logger::debug(string("Connection from "));
}

void Connection::closeSocket(int code, string_view str) {
	if (socketDisconnected) return;
	socketDisconnected = true;
	closeCode = code;
	closeReason = string(str);
	socket->end(code || CLOSE_ABNORMAL);
}

void Connection::onSocketMessage(string_view buffer) {
	if (!buffer.size() || buffer.size() > MAX_FRAME_SIZE) {
		closeSocket(CLOSE_TOO_LARGE, "Unexpected message size");
		return;
	}
}

void Connection::createPlayer() {
	Router::createPlayer();
	if (listener->handle->getSettingBool("chatEnabled"))
		listener->globalChat->add(this);
	if (listener->handle->getSettingBool("matchmakerNeedsQueuing")) {
		listener->globalChat->directMessage(nullptr, this, "This server requires players to be queued.");
		listener->globalChat->directMessage(nullptr, this, "Try spawning to enqueue.");
	} else {
		// TODO match maker toggle queued
	}
}

void Connection::onChatMessage(string_view message) {
	message
}

void Connection::onWorldSet() {
	protocol->onNewWorldBounds(&player->world->border, true);
}

void Connection::onWorldReset() {
	protocol->onWorldReset();
}

void Connection::onNewOwnedCell(PlayerCell* cell) {
	protocol->onNewOwnedCell(cell);
}

void Connection::send(string_view message) {
	if (socketDisconnected) return;
	socket->send(message);
}

void Connection::update() {
	if (!hasPlayer) return;
	if (!player->hasWorld) {
		if (spawningName.size()) {
			// TODO: match maker toggle queued
		}
		spawningName = "";
		splitAttempts = 0;
		ejectAttempts = 0;
		requestingSpectate = false;
		isPressingQ = false;
		hasPressedQ = false;
		return;
	}
	player->updateVisibleCells();

	vector<Cell*> add;
	vector<Cell*> upd;
	vector<Cell*> eat;
	vector<Cell*> del;

	for (auto pair : player->visibleCells) {
		if (!player->lastVisibleCells.contains(pair.first)) add.push_back(pair.second);
		else if (pair.second->shouldUpdate()) upd.push_back(pair.second);
	}

	for (auto pair : player->lastVisibleCells) {
		if (player->visibleCells.contains(pair.first)) continue;
		if (pair.second->eatenBy) eat.push_back(pair.second);
		del.push_back(pair.second);
	}
	
	if (player->state == PlayerState::SPEC || player->state == PlayerState::ROAM)
		protocol->onSpectatePosition(player->viewArea);
	if (listener->handle->tick % 4 == 0)
		listener->handle->gamemode->sendLeaderboard(this);
	protocol->onVisibleCellUpdate(add, upd, eat, del);
}