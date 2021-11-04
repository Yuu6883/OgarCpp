#pragma once
#include <atomic>
#include <chrono>
#include <vector>
#include <uwebsockets/App.h>
#include "../primitives/Logger.h"
#include "Listener.h"
#include "Router.h"

class Protocol;
class Minion;

using namespace std::chrono;
using std::atomic;
using std::vector;
using std::string;
using std::string_view;

enum CloseCodes : short {
	CLOSE_NORMAL = 1000,
	CLOSE_GOING_AWAY,
	CLOSE_PROTOCOL_ERROR,
	CLOSE_UNSUPPORTED,
	CLOSED_NO_STATUS = 1005,
	CLOSE_ABNORMAL,
	UNSUPPORTED_PAYLOAD,
	POLICY_VIOLATION,
	CLOSE_TOO_LARGE,
	MANDATORY_EXT,
	SERVER_ERROR,
	SERVER_RESTART
};

class Connection : public Router {
public:
	string ip;
	uWS::WebSocket<false, true, SocketData>* socket = nullptr;
	uWS::WebSocket<true,  true, SocketData>* SSLsocket = nullptr;
	time_point<steady_clock> lastChatTime = steady_clock::now();
	Protocol* protocol = nullptr;
	atomic<bool> socketDisconnected = false;
	int closeCode = 0;
	string closeReason = "";
	vector<Minion*> minions;
	bool minionsFrozen = false;
	bool controllingMinions = false;
	uWS::Loop* loop = nullptr;

	Connection(Listener* listener, string ip, uWS::WebSocket<false, true, SocketData>* socket) :
		Router(listener), ip(ip), socket(socket) {
		type = RouterType::PLAYER;
	};

	Connection(Listener* listener, string ip, uWS::WebSocket<true, true, SocketData>* SSLsocket) :
		Router(listener), ip(ip), SSLsocket(SSLsocket) {
		type = RouterType::PLAYER;
	};

	~Connection();
	bool isExternal() { return true; };
	bool isUTF16();
	void close();
	void onSocketClose(int code, string_view reason);
	void onSocketMessage(string_view buffer);
	void createPlayer();
	void onChatMessage(string_view message);
	bool shouldClose() { return socketDisconnected; };
	void onQPress();
	void update();
	void onWorldSet();
	void onNewOwnedCell(PlayerCell*);
	void onWorldReset();
	void send(string_view data, bool preserveBuffer = false);
	void closeSocket(int code, string_view reason);
	bool isThreaded();
	void onDead();
	void postUpdate();
};