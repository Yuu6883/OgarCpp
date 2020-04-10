
#include "Listener.h"
#include "Connection.h"
#include "../misc/Misc.h"
#include "../ServerHandle.h"
#include "../protocols/Protocol.h"
#include "../primitives/Logger.h"
#include "../worlds/MatchMaker.h"

#define MAX_FRAME_SIZE 512

string ipv4ToString(unsigned int ipv4) {
	return "";
}

Connection::~Connection() { if (protocol) delete protocol; };

void Connection::close() {
	if (!socketDisconnected) {
		closeSocket(CLOSE_GOING_AWAY, "Manual connection close call");
		return;
	}
	disconnected = true;
	disconnectedTick = listener->getTick();
	listener->onDisconnection(this, closeCode, closeReason);
}

// Called in socket thread
void Connection::onSocketClose(int code, string_view reason) {
	if (socketDisconnected) return;
	closeCode = code;
	closeReason = string(reason);
	socketDisconnected = true;
}

void Connection::closeSocket(int code, string_view str) {
	if (socketDisconnected) return;
	disconnected = true;
	disconnectedTick = listener->getTick();
	socketDisconnected = true;
	closeCode = code;
	closeReason = string(str);
	uWS::Loop::get()->defer([this, code, str] {
		socket->end(code ? code : CLOSE_ABNORMAL, str);
	});
}

void Connection::onSocketMessage(string_view buffer) {
	if (!buffer.size() || buffer.size() > MAX_FRAME_SIZE) {
		closeSocket(CLOSE_TOO_LARGE, "Unexpected message size");
		return;
	}
	Reader reader(buffer);
	if (protocol) protocol->onSocketMessage(reader);
	else {
		protocol = listener->handle->protocols->decide(this, reader);
		if (!protocol) closeSocket(CloseCodes::CLOSE_UNSUPPORTED, "Ambiguous protocol");
		else protocol->onDistinguished();
	}
}

void Connection::createPlayer() {
	Router::createPlayer();
	if (listener->handle->runtime.chatEnabled)
		listener->globalChat->add(this);
	if (listener->handle->runtime.matchmakerNeedsQueuing) {
		listener->globalChat->directMessage(nullptr, this, "This server requires players to be queued.");
		listener->globalChat->directMessage(nullptr, this, "Try spawning to enqueue.");
	} else {
		listener->handle->matchmaker.toggleQueued(this);
	}
}

void Connection::onChatMessage(string_view message) {
	string m = trim(string(message));
	if (!m.size()) return;
	auto lastChatTime = this->lastChatTime;
	this->lastChatTime = steady_clock::now();
	if (m.length() >= 2 && m[0] == '/') {
		m.erase(m.begin());
		if (!listener->handle->chatCommands.execute(listener->handle, m))
			listener->globalChat->directMessage(nullptr, this, "Unknown command, execute /help for the list of commands");
	} else if (duration_cast<milliseconds>(this->lastChatTime - lastChatTime).count() 
		>= listener->handle->runtime.chatCooldown) {
		listener->globalChat->broadcast(this, m);
	}
}

void Connection::onQPress() {
	if (!hasPlayer) return;
	if (listener->handle->runtime.minionEnableQBasedControl && minions.size())
		controllingMinions = !controllingMinions;
	else listener->handle->gamemode->onPlayerPressQ(player);
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
	if (socketDisconnected) {
		Logger::warn("Sending buffer but socket is disconnected");
		return;
	}
	socket->send(message);
}

void Connection::update() {
	if (!hasPlayer) return;
	if (!player->hasWorld) {
		if (requestSpawning)
			listener->handle->matchmaker.toggleQueued(this);
		spawningName = "";
		spawningSkin = "";
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
		if (!protocol->noDelDup || !pair.second->eatenBy) del.push_back(pair.second);
	}
	
	if (player->state == PlayerState::SPEC || player->state == PlayerState::ROAM)
		protocol->onSpectatePosition(&player->viewArea);
	if (listener->handle->tick % 4 == 0)
		listener->handle->gamemode->sendLeaderboard(this);
	protocol->onVisibleCellUpdate(add, upd, eat, del);
}