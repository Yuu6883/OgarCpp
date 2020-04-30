#pragma once

#include <string>
#include <string_view>
#include <list>

class Connection;
class Listener;

using std::list;
using std::string;
using std::string_view;

struct ChatSource {
	string name;
	bool isServer;
	unsigned int color;
	unsigned short pid;
	bool isUTF16;
	ChatSource(string name, bool isServer, unsigned int color, unsigned short pid = 0, bool isUTF16 = false) :
		name(name), isServer(isServer), color(color), pid(pid), isUTF16(isUTF16) {};
	static ChatSource from(Connection* conn);
};

struct ChatChannel {

	list<Connection*> connections;
	Listener* listener;

	ChatChannel(Listener* listener) : listener(listener) {};

	void add(Connection* conn) {
		connections.push_back(conn);
	}

	void remove(Connection* conn) {
		connections.remove(conn);
	}

	bool shouldFilter(string_view message);

	void broadcast(Connection* conn, string_view message);

	void directMessage(Connection* conn, Connection* recip, string_view message);
};