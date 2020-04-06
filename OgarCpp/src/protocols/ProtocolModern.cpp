#include "ProtocolModern.h"
#include "../primitives/Reader.h"
#include "../primitives/Writer.h"
#include "../sockets/Connection.h"
#include "../sockets/Listener.h"
#include "../worlds/Player.h"
#include "../cells/Cell.h"
#include "../ServerHandle.h"

static const char REEE[2] = { 0, 0 };
static const string_view PingReturn = string_view(REEE, 2);

void ProtocolModern::onSocketMessage(Reader& reader) {

	unsigned char messageId = reader.readUInt8();
	unsigned char count = 0;
	unsigned char globalFlags = 0;

	switch (messageId) {
		case 2:
			send(PingReturn);
			worldStatsPending = true;
			break;
		case 3:
			if (reader.length() < 12)
				return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message format");
			connection->mouseX = reader.readInt32();
			connection->mouseY = reader.readInt32();
			connection->splitAttempts += reader.readUInt8();
			count = reader.readUInt8();
			// TODO increment splitAttempts of minions

			globalFlags = reader.readUInt8();
			if (globalFlags & 1) {
				if (reader.length() < 13)
					return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message format");
				connection->spawningName = reader.readStringUTF8();
				connection->requestSpawning = true;
			}
			if (globalFlags & 2)  connection->requestingSpectate = true;
			if (globalFlags & 4)  connection->isPressingQ = true;
			if (globalFlags & 8)  connection->isPressingQ = connection->hasPressedQ = false;
			if (globalFlags & 16) connection->ejectAttempts++;
			// if (globalFlags & 32) 
			// TODO increment ejectAttempts of minions
			if (globalFlags & 64) connection->minionsFrozen = !connection->minionsFrozen;
			if (globalFlags & 128) {
				if (reader.length() < 13 + (globalFlags & 1))
					return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message format");
				count = reader.readUInt8();
				if (reader.length() < 13 + (globalFlags & 1) + count)
					return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message format");
				for (unsigned char i = 0; i < count; i++)
					connection->onChatMessage(reader.readStringUTF8());
			}
		default:
			return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message type");
	}
}

void ProtocolModern::onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del) {
	unsigned short globalFlags = 0;
	bool hitSelfData = false;
	unsigned char flags = 0;

	if (spectateAreaPending) 
		globalFlags |= 1;
	if (worldBorderPending)  
		globalFlags |= 2;
	if (serverInfoPending)   
		globalFlags |= 4;
	if (connection->hasPlayer && connection->player->hasWorld && worldStatsPending)
		globalFlags |= 8;
	if (chatPending.size())  
		globalFlags |= 8;
	if (leaderboardPending)  
		globalFlags |= 16;
	if (clearCellsPending) {
		globalFlags |= 32;
		clearCellsPending = false;
	}

	if (add.size())
		globalFlags |= 128;
	if (upd.size())
		globalFlags |= 256;
	if (eat.size())
		globalFlags |= 512;
	if (del.size())
		globalFlags |= 1024;

	if (!globalFlags) return;

	Writer writer;
	writer.writeUInt8(3);
	writer.writeUInt16(globalFlags);

	if (spectateAreaPending) {
		writer.writeFloat32(spectateAreaPending->getX());
		writer.writeFloat32(spectateAreaPending->getY());
		writer.writeFloat32(spectateAreaPending->s);
		spectateAreaPending = nullptr;
	}
	if (worldBorderPending) {
		auto r = worldBorderPending;
		writer.writeFloat32(r->getX() - r->w);
		writer.writeFloat32(r->getX() + r->w);
		writer.writeFloat32(r->getY() - r->h);
		writer.writeFloat32(r->getY() + r->h);
		worldBorderPending = nullptr;
	}
	if (serverInfoPending) {
		writer.writeUInt8(connection->listener->handle->gamemode->getType());
		writer.writeUInt8(OGAR_VERSION[0]);
		writer.writeUInt8(OGAR_VERSION[1]);
		writer.writeUInt8(OGAR_VERSION[2]);
		serverInfoPending = false;
	}
	if (worldStatsPending) {
		auto stats = &connection->player->world->stats;
		writer.writeStringUTF8(stats->name.data());
		writer.writeStringUTF8(stats->gamemode.data());
		writer.writeFloat32(stats->loadTime / connection->listener->handle->tickDelay);
		writer.writeUInt32(stats->uptime);
		writer.writeUInt16(stats->limit);
		writer.writeUInt16(stats->external);
		writer.writeUInt16(stats->internal);
		writer.writeUInt16(stats->playing);
		writer.writeUInt16(stats->spectating);
		worldStatsPending = false;
	}
	if (chatPending.size() > 0) {
		writer.writeUInt16(chatPending.size());
		for (auto chat_pair : chatPending) {
			writer.writeStringUTF8(chat_pair.first->name.data());
			writer.writeColor(chat_pair.first->color);
			writer.writeUInt8(chat_pair.first->isServer ? 1 : 0);
			writer.writeStringUTF8(chat_pair.second.data());
			delete chat_pair.first;
		}
		chatPending.clear();
	}
	if (leaderboardPending) {
		switch (lbType) {
			case LBType::FFA:
				writer.writeUInt8(1);
				for (auto item : lbData) {
					auto entry = (FFAEntry*) item;
					flags = 0;
					if (entry->highlighted) flags |= 1;
					if (entry == lbSelfData)
						flags |= 2, hitSelfData = true;
					writer.writeUInt16(entry->position);
					writer.writeUInt8(flags);
					writer.writeStringUTF8(entry->name.data());
				}
				writer.writeUInt8(0);
			case LBType::PIE:
				// TODO PIE LB
				break;
			case LBType::TEXT:
				// TODO TEXT LB
				break;
		}
		leaderboardPending = false;
		lbType = LBType::NONE;
	}

	if (add.size()) {
		for (auto cell : add) {
			writer.writeUInt32(cell->id);
			writer.writeUInt8(cell->getType());
			writer.writeFloat32(cell->getX());
			writer.writeFloat32(cell->getY());
			writer.writeUInt16(cell->getSize());
			writer.writeColor(cell->getColor());
			flags = 0;
			if (cell->getType() == PLAYER && cell->owner == connection->player)
				flags |= 1;
			auto name = cell->getName();
			auto skin = cell->getSkin();
			if (name.length())
				flags |= 2;
			if (skin.length())
				flags |= 4;
			writer.writeUInt8(flags);
			if (name.length())
				writer.writeStringUTF8(name.data());
			if (skin.length())
				writer.writeStringUTF8(skin.data());
		}
		writer.writeUInt32(0);
	}
	if (upd.size()) {
		for (auto cell : upd) {
			flags = 0;
			if (cell->posChanged)
				flags |= 1;
			if (cell->sizeChanged)
				flags |= 2;
			if (cell->colorChanged)
				flags |= 4;
			if (cell->nameChanged)
				flags |= 8;
			if (cell->skinChanged)
				flags |= 16;
			writer.writeUInt32(cell->id);
			writer.writeUInt8(flags);
			if (cell->posChanged) {
				writer.writeFloat32(cell->getX());
				writer.writeFloat32(cell->getY());
			}
			if (cell->sizeChanged)
				writer.writeUInt16(cell->getSize());
			if (cell->colorChanged)
				writer.writeColor(cell->getColor());
			if (cell->nameChanged)
				writer.writeStringUTF8(cell->getName().data());
			if (cell->skinChanged)
				writer.writeStringUTF8(cell->getSkin().data());
		}
		writer.writeUInt32(0);
	}
	if (eat.size()) {
		for (auto cell : eat) {
			writer.writeUInt32(cell->id);
			writer.writeUInt32(cell->eatenBy->id);
		}
		writer.writeUInt32(0);
	}
	if (del.size()) {
		for (auto cell : del)
			writer.writeUInt32(cell->id);
		writer.writeUInt32(0);
	}
	
	send(writer.finalize());
}