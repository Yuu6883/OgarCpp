#pragma once

#include <string>
#include <string_view>
#include <vector>

class Connection;
class Listener;

using std::string;
using std::string_view;
using std::vector;

struct ChatSource {

	string name;
	bool isServer;
	unsigned int color;

	ChatSource(string name, bool isServer, unsigned int color) :
		name(name), isServer(isServer), color(color) {};

	static ChatSource from(Connection* conn);
};

struct ChatChannel {

	vector<Connection*> connections;
	Listener* listener;

	ChatChannel(Listener* listener) : listener(listener) {};

	void add(Connection* conn) {
		connections.push_back(conn);
	}

	void remove(Connection* conn) {
		auto iter = connections.begin();
		while (iter != connections.cend()) {
			if (*iter == conn) {
				connections.erase(iter);
				return;
			}
			iter++;
		}
	}

	bool shouldFilter(string_view message);

	void broadcast(Connection* conn, string_view message);

	void directMessage(Connection* conn, Connection* recip, string_view message);
};