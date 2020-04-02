#pragma once

#include <string>
#include <string_view>
#include "../sockets/Connection.h"
#include "../primitives/Reader.h"
class Connection;
class Reader;

using std::string;
using std::string_view;

class Protocol {
public:
	Connection* connection;
	Protocol(Connection* connection) : connection(connection) {};
	virtual string getType() { return "Default Type"; };
	virtual string getSubtype() { return "Default Subtype"; };
	virtual bool distinguishes(Reader& reader) { return false; };
	virtual void onSocketMessage(Reader& reader) {};
	virtual void onChatMessage(ChatSource& source, string_view message) {};
	virtual void onNewOwnedCell(PlayerCell* cell) {};
	virtual void onNewWorldBounds(World* world, bool includeServerInfo) {};
	virtual void onWorldReset() {};
	virtual void onLeaderboardUpdate(string_view type) {};
	virtual void onSpectatePosition(ViewArea& viewArea) {};
	virtual void onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del) {};
	virtual void send(string_view data) {};
	void fail(int code, string_view reason) {
		connection->closeSocket(code || CLOSE_UNSUPPORTED, reason.size() ? reason : "Unspecified protocol fail");
	};
};