#pragma once

#include "../primitives/Rect.h"
#include "../sockets/ChatChannel.h"
#include "Protocol.h"

class Cell;
using std::pair;
using std::make_pair;

class ProtocolModern : public Protocol {
public:
	unsigned int protocol = -1;

	bool leaderboardPending = false;
	bool serverInfoPending = false;
	bool worldStatsPending = false;
	bool clearCellsPending = false;

	LBType lbType = LBType::NONE;
	vector<LBEntry*> lbData;
	LBEntry* lbSelfData = nullptr;

	vector<pair<ChatSource*, string>> chatPending;
	Rect* worldBorderPending = nullptr;
	ViewArea* spectateAreaPending = nullptr;

	ProtocolModern(Connection* connection) : Protocol(connection) {};
	string getType() { return "Modern"; };
	string getSubtype() { return string("m") + (protocol > 0 ? std::to_string(protocol) : "//"); };
	bool distinguishes(Reader& reader) {

		if (reader.length() < 5) return false;
		if (reader.readUInt8() != 1) return false;
		protocol = reader.readUInt32();
		if (protocol != 3) {
			fail(CloseCodes::CLOSE_UNSUPPORTED, "Unsupported protocol version");
			return false;
		}
		connection->createPlayer();
		return true;
	}
	void onDistinguished() {};
	void onSocketMessage(Reader& reader);
	void onChatMessage(ChatSource& source, string_view message) {
		chatPending.push_back(make_pair(new ChatSource(source), string(message)));
	}
	void onPlayerSpawned(Player* player) {};
	void onNewOwnedCell(PlayerCell* cell) { /* ignores it */ };
	void onNewWorldBounds(Rect* border, bool includeServerInfo) {
		worldBorderPending = border;
		serverInfoPending = includeServerInfo;
	};
	void onWorldReset() {
		clearCellsPending = true;
		worldBorderPending = nullptr;
		worldStatsPending = false;
		vector<Cell*> placeholder;
		onVisibleCellUpdate(placeholder, placeholder, placeholder, placeholder);
	};
	void onLeaderboardUpdate(LBType type, vector<LBEntry*>& entries, LBEntry* selfEntry) {
		leaderboardPending = true;
		lbType = type;

		while (lbData.size()) {
			delete lbData.back();
			lbData.pop_back();
		}

		lbData = entries;
		if (lbSelfData) delete lbSelfData;
		lbSelfData = selfEntry;
	};
	void onSpectatePosition(ViewArea* area) {
		spectateAreaPending = area;
	}
	void onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del);
	void onVisibleCellThreadedUpdate() {};
	Protocol* clone() { return new ProtocolModern(*this); };
	void onDead() {};
	void onMinimapUpdate() {};
};