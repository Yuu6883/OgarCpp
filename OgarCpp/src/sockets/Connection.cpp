
#include "Listener.h"
#include "Connection.h"
#include "../misc/Misc.h"
#include "../ServerHandle.h"
#include "../protocols/Protocol.h"
#include "../primitives/Logger.h"
#include "../worlds/MatchMaker.h"

#define MAX_FRAME_SIZE 1024
using std::to_string;

Connection::~Connection() { if (protocol) delete protocol; };

void Connection::close() {
	if (!socketDisconnected) {
		closeSocket(CLOSE_GOING_AWAY, "Server closed connection");
		return;
	}
	disconnected = true;
	disconnectedTick = listener->getTick();
}

bool Connection::isUTF16() { return protocol ? protocol->UTF16String : false; };

// Called in socket thread
void Connection::onSocketClose(int code, string_view reason) {
	if (socketDisconnected) return;
	closeCode = code;
	closeReason = string(reason);
	socketDisconnected = true;
}

void Connection::closeSocket(int code, string_view str) {
	if (socketDisconnected) return;
	socketDisconnected = true;
	closeCode = code;
	closeReason = string(str);
	loop->defer([this, code, str] {
		if (socket) socket->end(code ? code : CLOSE_ABNORMAL, str);
		else SSLsocket->end(code ? code : CLOSE_ABNORMAL, str);
	});
}

void Connection::onSocketMessage(string_view buffer) {
	if (!buffer.size() || buffer.size() > MAX_FRAME_SIZE) {
		closeSocket(CLOSE_TOO_LARGE, "Unexpected message size: " + to_string(buffer.size()));
		return;
	}
	Reader reader(buffer);
	if (protocol) protocol->onSocketMessage(reader);
	else {
		protocol = listener->handle->protocols->decide(this, reader);
		if (!protocol) closeSocket(CloseCodes::CLOSE_UNSUPPORTED, "Ambiguous protocol");
		else {
			protocol->onDistinguished();
			Logger::debug(string("Client connected using protocol: ") + protocol->getType());
		}
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
	Logger::info(string("[") + player->leaderboardName + "]: " + string(message));
	string m = trim(string(message));
	if (!m.size()) return;
	auto lastChatTime = this->lastChatTime;
	this->lastChatTime = steady_clock::now();
	if (m.length() >= 2 && m[0] == '/') {
		m.erase(m.begin());
		listener->handle->chatCommands.execute(listener->handle, m);
		// listener->globalChat->directMessage(nullptr, this, "Unknown command, execute /help for the list of commands");
	} else if (duration_cast<milliseconds>(this->lastChatTime - lastChatTime).count() 
		>= listener->handle->runtime.chatCooldown) {
		listener->globalChat->broadcast(this, m);
	} else {
		listener->globalChat->directMessage(nullptr, this, "You are entering too fast");
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

void Connection::send(string_view message, bool preserveBuffer) {
	if (socketDisconnected) {
		Logger::warn("Sending buffer but socket is disconnected");
		if (!preserveBuffer) delete message.data();
		return;
	}
	loop->defer([this, message, preserveBuffer] {
		listener->handle->bytesSent += message.size();
		bool backpressure = socket ? socket->send(message) : SSLsocket->send(message);
		if (!backpressure) {
			busy = true;
			int bufferedAmount = socket ? socket->getBufferedAmount() : SSLsocket->getBufferedAmount();
			if (player)
				Logger::warn(player->leaderboardName + " backpressure alert: " + to_string(bufferedAmount) + ")");
		}
		if (!preserveBuffer) delete message.data();
	});
}

bool Connection::isThreaded() {
	return protocol ? protocol->threadedUpdate : false;
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

	if (busy) return;
	if (!protocol) return;

	// No need to do thread update, spectate target will send buffer to this router
	if (player->state == PlayerState::SPEC) return;

	auto iter = spectators.begin();
	while (iter != spectators.end()) {
		auto router = *iter;
		if (router->hasPlayer && router->spectateTarget != this)
			iter = spectators.erase(iter);
		iter++;
	}

	player->updateVisibleCells();

	vector<Cell*> add;
	vector<Cell*> upd;
	vector<Cell*> eat;
	vector<Cell*> del;

	for (auto [id, cell] : player->visibleCells) {
		if (player->lastVisibleCells.find(id) == player->lastVisibleCells.cend()) add.push_back(cell);
		else if (cell->shouldUpdate()) upd.push_back(cell);
	}

	for (auto [id, cell] : player->lastVisibleCells) {
		if (player->visibleCells.find(id) != player->visibleCells.cend()) continue;
		if (cell->eatenBy) eat.push_back(cell);
		if (!protocol->noDelDup || !cell->eatenBy) del.push_back(cell);
		if (cell->deadTick == 1) del.push_back(cell); // delete dead player cell
	}

	if (player->state == PlayerState::SPEC || player->state == PlayerState::ROAM)
		protocol->onSpectatePosition(&player->viewArea);
	if (listener->handle->tick % 4 == 0)
		listener->handle->gamemode->sendLeaderboard(this);
	if (listener->handle->tick % 5 == 0)
		protocol->onMinimapUpdate();

	protocol->onTimingMatrix();
	if (player->state == PlayerState::SPEC && player->router->spectateTarget) {
		if (player->router->spectateTarget->type == RouterType::PLAYER &&
			player->router->spectateTarget->player->state == PlayerState::ALIVE) return;
	}
	protocol->onVisibleCellUpdate(add, upd, eat, del);
}

void Connection::onDead() {
	if (protocol) protocol->onDead();
}

void Connection::postUpdate() {
	if (protocol) protocol->postUpdate();
}