#include "ChatChannel.h"
#include "Connection.h"
#include "../protocols/Protocol.h"
#include "../worlds/Player.h"
#include <bit>

ChatSource ChatSource::from(Connection* conn) {
	return ChatSource(conn->player->chatName, false, conn->player->chatColor, conn->player->id, conn->isUTF16());
}

static void toLowerCase(string* str) {
	std::transform(str->begin(), str->end(), str->begin(), [](unsigned char c) { return tolower(c); });
};

const ChatSource serverSource = ChatSource("Server", true, 0x3F3FC0, 0, false);

bool ChatChannel::shouldFilter(string_view message) {
	return false;
}

void UTF8toUTF16(string_view& utf8, char* dist) {
	auto ptr = utf8.data();
	auto s = utf8.size();
	for (int i = 0; i < s; i++)
		dist[2 * i] = ptr[i];
}

void UTF16toUTF8(string_view& utf16, char* dist) {
	auto ptr = utf16.data();
	auto s = utf16.size();
	for (int i = std::endian::native == std::endian::little ? 0 : 1; i < s; i += 2)
		dist[i / 2] = ptr[i];
}


void ChatChannel::broadcast(Connection* conn, string_view message) {
	if (shouldFilter(message)) return;
	auto source = conn ? ChatSource::from(conn) : serverSource;

	string_view utf8;
	string_view utf16;
	char* temp = nullptr;

	if (source.isUTF16) {
		utf16 = message;
		temp = (char*) malloc(message.size() / 2);
		UTF16toUTF8(utf16, temp);
		utf8 = string_view(temp, message.size() / 2);
	} else {
		utf8 = message;
		temp = (char*) malloc(message.size() * 2);
		UTF8toUTF16(utf8, temp);
		utf16 = string_view(temp, message.size() * 2);
	}

	for (auto recip : connections)
		recip->protocol->onChatMessage(source, recip->isUTF16() ? utf16 : utf8);

	free(temp);
}

void ChatChannel::directMessage(Connection* conn, Connection* recip, string_view message) {
	if (!recip || !recip->protocol) return;
	if (shouldFilter(message)) return;
	auto source = conn ? ChatSource::from(conn) : serverSource;

	if (source.isUTF16 == recip->isUTF16()) {
		recip->protocol->onChatMessage(source, message);
	} else if (source.isUTF16) {
		auto temp = (char*) malloc(message.size() / 2);
		UTF16toUTF8(message, temp);
		auto utf8 = string_view(temp, message.size() / 2);
		recip->protocol->onChatMessage(source, utf8);
		free(temp);
	} else {
		auto temp = (char*) malloc(message.size() * 2);
		UTF8toUTF16(message, temp);
		auto utf16 = string_view(temp, message.size() * 2);
		recip->protocol->onChatMessage(source, utf16);
		free(temp);
	}
}