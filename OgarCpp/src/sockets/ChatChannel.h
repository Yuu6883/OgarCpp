#pragma once
#include <string>
#include <vector>
#include "Connection.h"

using std::string;
using std::vector;

static struct ChatSource {
	string name;
	bool isServer;
	unsigned int color;

	static ChatSource from(Connection* connection) {
		return {
			connection->player->chatName,
			false,
			connection->player->chatColor
		};
	}
};

static void toLowerCase(string_view& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return tolower(c); });
};

static const ChatSource serverSource = ChatSource{ "Server", true, 0x3F3FC0 };

struct ChatChannel {

	vector<Connection*> connections;
	Listener* listener;
	ChatChannel(Listener* listener) : listener(listener) {};

	void add(Connection* connection) {
		connections.push_back(connection);
	}

	void remove(Connection* connection) {
		auto iter = connections.begin();
		auto cend = connections.cend();
		while (iter != cend) {
			if (*iter == connection) {
				connections.erase(iter);
				return;
			}
		}
	}

	bool shouldFilter(string_view message) {
		// TODO: filter chat
		return false;
	}

	void broadcast(Connection* conn, string_view message) {
		if (shouldFilter(message)) return;
		auto source = conn ? ChatSource::from(conn) : serverSource;
		// TODO broadcast to protocol instances
	}

	void directMessage(Connection* conn, Connection* recip, string_view message) {
		if (shouldFilter(message)) return;
		auto source = conn ? ChatSource::from(conn) : serverSource;
		// TODO dm to the protocol instance
	}
};