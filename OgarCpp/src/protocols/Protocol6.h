#pragma once

#include "../misc/Misc.h"
#include "../primitives/Rect.h"
#include "../sockets/ChatChannel.h"
#include "Protocol.h"

class Cell;
class PlayerCell;
using std::pair;
using std::make_pair;

class Protocol6 : public Protocol {
public:
	unsigned int protocol = 6;
	bool gotKey = false;
	unsigned int key = 0;

	LBType lastLbType = LBType::NONE;

	Protocol6(Connection* connection) : Protocol(connection) {};
	string getType() { return "Legacy"; };
	string getSubtype() { return string("m") + (protocol > 0 ? std::to_string(protocol) : "//"); };
	bool distinguishes(Reader& reader) {

		if (reader.length() < 5) return false;
		if (reader.readUInt8() != 254) return false;
		protocol = reader.readUInt32();
		if (protocol != 6) {
			fail(CloseCodes::CLOSE_UNSUPPORTED, "Unsupported protocol version");
			return false;
		}
		return true;
	}
	void onDistinguished() {};
	void onSocketMessage(Reader& reader);
	void onChatMessage(ChatSource& source, string_view message);
	void onNewOwnedCell(PlayerCell* cell);
	void onPlayerSpawned(Player* player) {};
	void onNewWorldBounds(Rect* border, bool includeServerInfo);
	void onWorldReset();
	void onLeaderboardUpdate(LBType type, vector<LBEntry*>& entries, LBEntry* selfEntry);
	void onSpectatePosition(ViewArea* area);
	void onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del);
	Protocol* clone() { return new Protocol6(*this); };
	void onStatsRequest();
};