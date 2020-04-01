#pragma once

#include <chrono>
#include <uwebsockets/App.h>
#include "Router.h"

using namespace std::chrono;
using std::string;
using std::string_view;

class Connection : public Router {
public:
	int ipv4;
	uWS::WebSocket<false, true>* socket;
	time_point<steady_clock> lastChatTime = steady_clock::now();
	bool socketDisconnected = false;
	int closeCode = 0;
	string closeReason = "";
	// Minions
	bool minionsFrozen = false;
	bool controllingMinions = false;

	Connection(Listener* listener, int ipv4, uWS::WebSocket<false, true>* socket) :
		Router(listener), ipv4(ipv4), socket(socket) {};
	bool isExternal() override { return true; };
	void close();
	void onSocketClose(int code, string_view reason);
	void onSocketMessage(string_view buffer);
	void createPlayer();
	void onChatMessage(string_view message);
	void onQPress();
	bool shouldClose() { return socketDisconnected; };
	void update();
	void onWorldSet() {};
	void onNewOwnedCell(PlayerCell*);
	void onWorldReset();
	void send(string_view data);
	void closeSocket(int code, string_view reason);
};