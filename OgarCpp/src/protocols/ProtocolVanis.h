#pragma once

#include "Protocol.h"
#include "../misc/Misc.h"
#include "../primitives/Rect.h"
#include "../sockets/ChatChannel.h"

class Cell;
class PlayerCell;
using std::make_pair;
using std::string_view;

class ProtocolVanis : public Protocol {
public:
	ProtocolVanis(Connection* connection) : Protocol(connection) {
		noDelDup = true;
		// UTF16String = true;
		// threadedUpdate = true;
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
	void onMinimapUpdate();
	void onDead();
	Protocol* clone() { return new ProtocolVanis(*this); };
};