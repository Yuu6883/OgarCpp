#include <iostream>
#include "Protocol6.h"
#include "../primitives/Reader.h"
#include "../primitives/Writer.h"
#include "../sockets/Connection.h"
#include "../sockets/Listener.h"
#include "../worlds/Player.h"
#include "../cells/Cell.h"
#include "../ServerHandle.h"

void Protocol6::onSocketMessage(Reader& reader) {

	unsigned char messageId = reader.readUInt8();

	if (!gotKey) {
		if (messageId != 255) return;
		if (reader.length() < 5) return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message format");
		gotKey = true;
		key = reader.readUInt32();
		connection->createPlayer();
		return;
	}

	unsigned char flags = 0;
	int skipLen = 0;

	switch (messageId) {
		case 0:
			connection->spawningName = reader.readStringUTF8();
			connection->requestSpawning = true;
			break;
		case 1:
			connection->requestingSpectate = true;
			break;
		case 16:
			switch (reader.length()) {
				case 13:
					connection->mouseX = reader.readInt32();
					connection->mouseY = reader.readInt32();
					break;
				case 9:
					connection->mouseX = reader.readInt16();
					connection->mouseY = reader.readInt16();
					break;
				case 21:
					connection->mouseX = reader.readInt64();
					connection->mouseY = reader.readInt64();
					break;
				default:
					return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message format");
			}
			break;
		case 17:
			if (connection->controllingMinions) {
				// TODO split minions
			} else connection->splitAttempts++;
			break;
		case 18: connection->isPressingQ = true; break;
		case 19: connection->isPressingQ = connection->hasPressedQ = false; break;
		case 21: 
			if (connection->controllingMinions) {
				// TODO eject minions
			} else connection->ejectAttempts++;
			break;
		case 22:
			// ?????????????????
			break;
		case 23:
			// ?????????????????
			break;
		case 24:
			// ?????????????????
			break;
		case 99:
			if (reader.length() < 2)
				return fail(CloseCodes::CLOSE_UNSUPPORTED, "Bad message format");
			flags = reader.readUInt8();
			skipLen = 2 * ((flags & 2) + (flags & 4) + (flags & 8));
			if (reader.length() < 2 + skipLen)
				return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message type");
			reader.skip(skipLen);
			connection->onChatMessage(reader.readStringUTF8());
			break;
		case 254:
			if (connection->hasPlayer && connection->player->hasWorld)
				onStatsRequest();
			break;
		case 255:
			return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unexpected message type");
		default:
			return fail(CloseCodes::CLOSE_UNSUPPORTED, "Unknown message type");
	}
}

void Protocol6::onVisibleCellUpdate(vector<Cell*>& add, vector<Cell*>& upd, vector<Cell*>& eat, vector<Cell*>& del) {
	auto player = connection->player;
	Writer writer;
	writer.writeUInt8(16);

	writer.writeUInt16(eat.size());
	for (auto cell : eat) {
		writer.writeUInt32(cell->eatenBy->id);
		writer.writeUInt32(cell->id);
	}

	for (auto cell : add) {
		writer.writeUInt32(cell->id);
		writer.writeInt32(cell->getX());
		writer.writeInt32(cell->getY());
		writer.writeUInt16(cell->getSize());

		unsigned char flags = 0;
		if (cell->isSpiked()) flags |= 0x01;
		flags |= 0x02;
		flags |= 0x04;
		flags |= 0x08;
		if (cell->isAgitated()) flags |= 0x10;
		if (cell->getType() == CellType::MOTHER_CELL) flags |= 0x20;
		writer.writeUInt8(flags);

		writer.writeColor(cell->getColor());
		writer.writeStringUTF8(cell->getSkin().data());
		writer.writeStringUTF8(cell->getName().data());
	}

	for (auto cell : upd) {
		writer.writeUInt32(cell->id);
		writer.writeInt32(cell->getX());
		writer.writeInt32(cell->getY());
		writer.writeUInt16(cell->getSize());
		unsigned char flags = 0;
		if (cell->isSpiked()) flags |= 0x01;
		if (cell->colorChanged) flags |= 0x02;
		if (cell->skinChanged) flags |= 0x04;
		if (cell->nameChanged) flags |= 0x08;
		if (cell->isAgitated()) flags |= 0x10;
		if (cell->getType() == CellType::MOTHER_CELL) flags |= 0x20;
		writer.writeUInt8(flags);

		if (cell->colorChanged) writer.writeColor(cell->getColor());
		if (cell->skinChanged)  writer.writeStringUTF8(cell->getSkin().data());
		if (cell->nameChanged)  writer.writeStringUTF8(cell->getName().data());
	}
	writer.writeUInt32(0);

	writer.writeUInt16(del.size());
	for (auto cell : del) writer.writeUInt32(cell->id);
	send(writer.finalize());
}

static thread_local char stats_buffer[200];
static const char STATS_JSON_FORMAT[] = "{\"mode\":\"%s\", \"update\":%f, \"playersTotal\":%i, \"playersAlive\":%i, \"playersSpect\":%i, \"playersLimit\":%i }";

void Protocol6::onStatsRequest() {
	Writer writer;
	writer.writeUInt8(254);
	auto stats = &connection->player->world->stats;
	int written = snprintf(stats_buffer, sizeof(stats_buffer), STATS_JSON_FORMAT, stats->gamemode.c_str(), stats->loadTime,
		stats->external, stats->playing, stats->spectating, stats->limit);
	if (written) {
		string_view jsonString(stats_buffer, written);
		writer.writeStringUTF8(jsonString.data());
		send(writer.finalize());
	} else {
		Logger::warn("Failed to write stats string");
	}
}

void Protocol6::onNewOwnedCell(PlayerCell* cell) {
	Writer writer;
	writer.writeUInt8(32);
	writer.writeUInt32(cell->id);
	send(writer.finalize());
};

void Protocol6::onNewWorldBounds(Rect* border, bool includeServerInfo) {
	Writer writer;
	writer.writeUInt8(64);
	writer.writeFloat64(border->getX() - border->w);
	writer.writeFloat64(border->getY() - border->h);
	writer.writeFloat64(border->getX() + border->w);
	writer.writeFloat64(border->getY() + border->h);
	if (includeServerInfo) {
		writer.writeUInt32(connection->listener->handle->gamemode->getType());
		writer.writeStringUTF8((string("OgarII(CPP)") + OGAR_VERSION_STRING).data());
	}
	send(writer.finalize());
};

void Protocol6::onWorldReset() {
	Writer writer;
	writer.writeUInt8(18);
	send(writer.finalize());
	if (lastLbType != LBType::NONE) {
		vector<LBEntry*> placeholder;
		onLeaderboardUpdate(lastLbType, placeholder, nullptr);
		lastLbType = LBType::NONE;
	}
};

void Protocol6::onLeaderboardUpdate(LBType type, vector<LBEntry*>& entries, LBEntry* selfEntry) {
	lastLbType = type;
	Writer writer;
	switch (type) {
		case LBType::FFA:
			writer.writeUInt8(49);
			writer.writeUInt32(entries.size());
			for (auto entry : entries) {
				writer.writeUInt32(((FFAEntry*)entry)->highlighted ? 1 : 0);
				writer.writeStringUTF8(((FFAEntry*)entry)->name.data());
			}
			break;
		case LBType::PIE:
			// no
			break;
		case LBType::TEXT:
			// no
			break;
	}
	send(writer.finalize());
}

void Protocol6::onSpectatePosition(ViewArea* area) {
	Writer writer;
	writer.writeUInt8(17);
	writer.writeFloat32(area->getX());
	writer.writeFloat32(area->getY());
	writer.writeFloat32(area->s);
	send(writer.finalize());
}

void Protocol6::onChatMessage(ChatSource& source, string_view message) {
	Writer writer;
	writer.writeUInt8(99);
	writer.writeUInt8(source.isServer ? 128 : 0);
	writer.writeColor(source.color);
	writer.writeStringUTF8(source.name.data());
	writer.writeStringUTF8(message.data());
	send(writer.finalize());
}