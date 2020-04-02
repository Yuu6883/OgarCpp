#pragma once

#include <string>
#include <string_view>
#include "../sockets/Connection.h"
#include "../primitives/Reader.h"
class Connection;
class Reader;

using std::string;
using std::string_view;

enum class LBType {
	FFA, PIE, TEXT
};

struct LBEntry {};

struct FFAEntry : LBEntry {
	string name;
	bool highlighted;
	unsigned int cellId;
	unsigned char position;
};

struct PIEEntry : LBEntry {
	double weight;
	unsigned int color;
};

struct TEXTENtry : LBEntry {
	string text;
};


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
	virtual void onNewWorldBounds(Rect* border, bool includeServerInfo) {};
	virtual void onWorldReset() {};
	virtual void onLeaderboardUpdate(LBType type, vector<LBEntry*>& entries, LBEntry* selfEntry) {};
	virtual void onSpectatePosition(ViewArea& viewArea) {};
	virtual void onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del) {};
	virtual void send(string_view data) {};
	void fail(int code, string_view reason) {
		connection->closeSocket(code || CLOSE_UNSUPPORTED, reason.size() ? reason : "Unspecified protocol fail");
	};
};