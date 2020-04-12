#pragma once

#include "Protocol.h"

#pragma once

#include "../misc/Misc.h"
#include "../primitives/Rect.h"
#include "../sockets/ChatChannel.h"
#include "Protocol.h"

class Cell;
class PlayerCell;
using std::pair;
using std::make_pair;

class ProtocolVanis : public Protocol {
public:
	bool leaderboardPending = false;
	bool serverInfoPending = false;
	bool worldStatsPending = false;
	bool clearCellsPending = false;

	LBType lbType = LBType::NONE;
	vector<LBEntry*> lbData;
	LBEntry* lbSelfData = nullptr;

	vector<pair<ChatSource*, string>> pendingChat;
	Rect* worldBorderPending = nullptr;
	ViewArea* spectateAreaPending = nullptr;

	ProtocolVanis(Connection* connection) : Protocol(connection) {
		noDelDup = true;
		threadedUpdate = true;
	};
	string getType() { return "Vanis"; };
	string getSubtype() { return "(XDDDD)"; };
	bool distinguishes(Reader& reader) {
		if (reader.length() != 4) return false;
		if (reader.readUInt16() != 69) return false;
		if (reader.readUInt16() != 420) return false;
		return true;
	}
	void onDistinguished() {
		connection->createPlayer();
	}
	void onSocketMessage(Reader& reader);
	void onChatMessage(ChatSource& source, string_view message);
	void onPlayerSpawned(Player* player);
	void onNewOwnedCell(PlayerCell* cell);
	void onNewWorldBounds(Rect* border, bool includeServerInfo);
	void onWorldReset();
	void onLeaderboardUpdate(LBType type, vector<LBEntry*>& entries, LBEntry* selfEntry);
	void onSpectatePosition(ViewArea* area);
	void onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del);
	void onStatsRequest();
	void onVisibleCellThreadedUpdate();
	Protocol* clone() { return new ProtocolVanis(*this); };
	void onDead();
};