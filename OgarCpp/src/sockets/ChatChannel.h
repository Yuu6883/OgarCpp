#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "Router.h"
class Router;

using std::string;
using std::string_view;
using std::vector;

struct ChatSource {

	string name;
	bool isServer;
	unsigned int color;

	ChatSource(string name, bool isServer, unsigned int color) :
		name(name), isServer(isServer), color(color) {};

	static ChatSource from(Router* router);
};

struct ChatChannel {

	vector<Router*> connections;
	Listener* listener;

	ChatChannel(Listener* listener) : listener(listener) {};

	void add(Router* router) {
		connections.push_back(router);
	}

	void remove(Router* router) {
		auto iter = connections.begin();
		while (iter != connections.cend()) {
			if (*iter == router) {
				connections.erase(iter);
				return;
			}
			iter++;
		}
	}

	bool shouldFilter(string_view message);

	void broadcast(Router* conn, string_view message);

	void directMessage(Router* conn, Router* recip, string_view message);
};