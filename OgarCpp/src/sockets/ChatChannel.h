#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "Router.h"

using std::string;
using std::string_view;
using std::vector;

static struct ChatSource {
	string name;
	bool isServer;
	unsigned int color;

	static ChatSource from(Router* router) {
		return {
			router->player->chatName,
			false,
			router->player->chatColor
		};
	}
};

static void toLowerCase(string_view& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return tolower(c); });
};

static const ChatSource serverSource = ChatSource{ "Server", true, 0x3F3FC0 };

struct ChatChannel {

	vector<Router*> connections;
	Listener* listener;
	ChatChannel(Listener* listener) : listener(listener) {};

	void add(Router* router) {
		connections.push_back(router);
	}

	void remove(Router* router) {
		auto iter = connections.begin();
		auto cend = connections.cend();
		while (iter != cend) {
			if (*iter == router) {
				connections.erase(iter);
				return;
			}
		}
	}

	bool shouldFilter(string_view message) {
		// TODO: filter chat
		return false;
	}

	void broadcast(Router* conn, string_view message) {
		if (shouldFilter(message)) return;
		auto source = conn ? ChatSource::from(conn) : serverSource;
		// TODO broadcast to protocol instances
	}

	void directMessage(Router* conn, Router* recip, string_view message) {
		if (shouldFilter(message)) return;
		auto source = conn ? ChatSource::from(conn) : serverSource;
		// TODO dm to the protocol instance
	}
};