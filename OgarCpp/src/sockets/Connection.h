#pragma once
#include <atomic>
#include <chrono>
#include <vector>
#include <uwebsockets/App.h>
#include "../primitives/Logger.h"
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
	CLOSE_TOO_LARGE
};

class Connection : public Router {
public:
	unsigned int ipv4;
	uWS::WebSocket<false, true>* socket;
	time_point<steady_clock> lastChatTime = steady_clock::now();
	Protocol* protocol = nullptr;
	atomic<bool> socketDisconnected = false;
	int closeCode = 0;
	string closeReason = "";
	vector<Minion*> minions;
	bool minionsFrozen = false;
	bool controllingMinions = false;

	Connection(Listener* listener, unsigned int ipv4, uWS::WebSocket<false, true>* socket) :
		Router(listener), ipv4(ipv4), socket(socket) {
		type = RouterType::PLAYER;
	};
	~Connection() { delete protocol; }
	bool isExternal() { return true; };
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
	void send(string_view data);
	void closeSocket(int code, string_view reason);
};