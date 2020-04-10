#pragma once

#include <list>
#include <algorithm>

using std::list;

class Connection;
class ServerHandle;
class World;

class MatchMaker {
public:
	ServerHandle* handle;
	list<Connection*> queued;
	MatchMaker(ServerHandle* handle) : handle(handle) {};
	bool isInQueue(Connection* connection) {
		return std::find(queued.begin(), queued.end(), connection) != queued.end();
	}
	void broadcastQueueLength();
	void toggleQueued(Connection* connection) {
		isInQueue(connection) ? dequeue(connection) : enqueue(connection);
	}
	void enqueue(Connection* connection);
	void dequeue(Connection* connection);
	void update();
	World* getSuitableWorld();
};