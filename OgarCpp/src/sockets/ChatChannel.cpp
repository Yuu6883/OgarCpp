#include "ChatChannel.h"

ChatSource ChatSource::from(Router* router) {
	return ChatSource(router->player->chatName, false, router->player->chatColor);
}

static void toLowerCase(string* str) {
	std::transform(str->begin(), str->end(), str->begin(), [](unsigned char c) { return tolower(c); });
};

const ChatSource serverSource = ChatSource("Server", true, 0x3F3FC0);

bool ChatChannel::shouldFilter(string_view message) {
	return false;
}

void ChatChannel::broadcast(Router* conn, string_view message) {
	if (shouldFilter(message)) return;
	auto source = conn ? ChatSource::from(conn) : serverSource;
	// TODO broadcast to protocol instances
}

void ChatChannel::directMessage(Router* conn, Router* recip, string_view message) {
	if (shouldFilter(message)) return;
	auto source = conn ? ChatSource::from(conn) : serverSource;
	// TODO dm to the protocol instance
}