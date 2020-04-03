#include "ChatChannel.h"
#include "Connection.h"
#include "../protocols/Protocol.h"
#include "../worlds/Player.h"

ChatSource ChatSource::from(Connection* conn) {
	return ChatSource(conn->player->chatName, false, conn->player->chatColor);
}

static void toLowerCase(string* str) {
	std::transform(str->begin(), str->end(), str->begin(), [](unsigned char c) { return tolower(c); });
};

const ChatSource serverSource = ChatSource("Server", true, 0x3F3FC0);

bool ChatChannel::shouldFilter(string_view message) {
	return false;
}

void ChatChannel::broadcast(Connection* conn, string_view message) {
	if (shouldFilter(message)) return;
	auto source = conn ? ChatSource::from(conn) : serverSource;
	for (auto recip : connections)
		recip->protocol->onChatMessage(source, message);
}

void ChatChannel::directMessage(Connection* conn, Connection* recip, string_view message) {
	if (shouldFilter(message)) return;
	auto source = conn ? ChatSource::from(conn) : serverSource;
	recip->protocol->onChatMessage(source, message);
}