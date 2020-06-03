#pragma once

#include <list>
#include <string>
#include <string_view>
#include "../primitives/Rect.h"
#include "../sockets/Connection.h"
#include "../primitives/Reader.h"

class Connection;
struct ChatSource;
class Reader;
class Cell;

using std::string;
using std::string_view;
using std::pair;

enum class LBType {
	NONE, FFA, PIE, TEXT
};

struct LBEntry {};

struct FFAEntry : LBEntry {
	string name;
	bool highlighted;
	unsigned int cellId;
	unsigned int pid;
	unsigned short position;
};

struct PIEEntry : LBEntry {
	float weight;
	unsigned int color;
};

struct TEXTENtry : LBEntry {
	string text;
};


class Protocol {
public:
	list<pair<string_view, bool>> pendingBuffer;
	bool noDelDup = false;
	bool threadedUpdate = false;
	bool UTF16String = false;
	Connection* connection;
	Protocol(Connection* connection) : connection(connection) {};
	~Protocol() {};
	virtual string getType() = 0;
	virtual string getSubtype() = 0;
	virtual bool distinguishes(Reader& reader) = 0;
	virtual void onDistinguished() = 0;
	virtual void onSocketMessage(Reader& reader) = 0;
	virtual void onChatMessage(ChatSource& source, string_view message) = 0;
	virtual void onPlayerSpawned(Player* player) = 0;
	virtual void onNewOwnedCell(PlayerCell* cell) = 0;
	virtual void onNewWorldBounds(Rect* border, bool includeServerInfo) = 0;
	virtual void onWorldReset() = 0;
	virtual void onLeaderboardUpdate(LBType type, vector<LBEntry*>& entries, LBEntry* selfEntry) = 0;
	virtual void onMinimapUpdate() = 0;
	virtual void onSpectatePosition(ViewArea* viewArea) = 0;
	virtual void onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del) = 0;
	virtual void onVisibleCellThreadedUpdate() = 0;
	virtual void onDead() = 0;
	void send(string_view data, bool preserveBuffer = false) { pendingBuffer.push_back(make_pair(data, preserveBuffer)); };
	void fail(int code, string_view reason) {
		connection->closeSocket(code ? code : CLOSE_UNSUPPORTED, reason.size() ? reason : "Unspecified protocol fail");
	};
	void postUpdate() {
		for (auto [data, preserveBuffer] : pendingBuffer)
			connection->send(data, preserveBuffer);
		pendingBuffer.clear();
	};
	virtual void onTimingMatrix() = 0;
	virtual Protocol* clone() = 0;
};