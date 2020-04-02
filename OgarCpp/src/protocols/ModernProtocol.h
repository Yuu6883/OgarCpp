#pragma once

#include "../primitives/Rect.h"
#include "../sockets/ChatChannel.h"
#include "Protocol.h"

class Cell;
using std::pair;
using std::make_pair;

class ModernProtocol : public Protocol {
public:
	Connection* connection;
	unsigned int protocol = -1;
	bool leaderboardPending = false;

	LBType lbType;
	vector<LBEntry*> lbData;
	LBEntry* lbSelfData = nullptr;

	vector<pair<ChatSource*, string>> chatPending;

	Rect* worldBorderPending = nullptr;
	ViewArea* spectateAreaPending = nullptr;

	bool serverInfoPending = false;
	bool worldStatsPending = false;
	bool clearCellsPending = false;

	ModernProtocol(Connection* connection) : Protocol(connection) {};
	virtual string getType() override { return "Modern"; };
	virtual string getSubtype() override { return string("m") + (protocol > 0 ? std::to_string(protocol) : "//"); };
	virtual bool distinguishes(Reader& reader) override {
		if (reader.length() < 5) return false;
		if (reader.readFloat32.readUInt8() != 1) return false;
		protocol = reader.readUInt32();
		if (protocol != 3) {
			fail(CloseCodes::CLOSE_UNSUPPORTED, "Unsupported protocol version");
			return false;
		}
		connection->createPlayer();
		return true;
	}
	virtual void onSocketMessage(Reader& reader) override;
	virtual void onChatMessage(ChatSource& source, string message) {
		chatPending.push_back(make_pair(new ChatSource(source), message));
	}
	virtual void onNewWorldBounds(Rect* border, bool includeServerInfo) override {
		worldBorderPending = border;
		serverInfoPending = includeServerInfo;
	};
	virtual void onWorldReset() {
		clearCellsPending = true;
		worldBorderPending = false;
		worldStatsPending = false;
		vector<Cell*> placeholder;
		onVisibleCellUpdate(placeholder, placeholder, placeholder, placeholder);
	};
	virtual void onLeaderboardUpdate(LBType type, vector<LBEntry*>& entries, LBEntry* selfEntry) override {
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
	virtual void onSpectatePosition(ViewArea* area) {
		spectateAreaPending = area;
	}
	virtual void onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del) override;
};