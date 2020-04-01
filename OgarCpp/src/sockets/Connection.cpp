#include "Connection.h"
#define MAX_FRAME_SIZE 512

enum CloseCodes : short {
	CLOSE_NORMAL = 1000,
	CLOSE_GOING_AWAY,
	CLOSE_PROTOCOL_ERROR,
	CLOSE_UNSUPPORTED,
	CLOSED_NO_STATUS = 1005,
	CLOSE_ABNORMAL,
	UNSUPPORTED_PAYLOAD,
	POLICY_VIOLATION,
	CLOSE_TOO_LARGE
};

void Connection::close() {
	if (!socketDisconnected) {
		closeSocket(CLOSE_GOING_AWAY, "Manual connection close call");
		return;
	}
	Router::close();
	disconnected = true;
	disconnectedTick = listener->getTick();
	listener->onDisconnection(this, closeCode, closeReason);
}

void Connection::closeSocket(int code, string_view str) {
	if (socketDisconnected) return;
	socketDisconnected = true;
	closeCode = code;
	closeReason = string(str);
	socket->end(code || CLOSE_ABNORMAL);
}

void Connection::onSocketMessage(string_view buffer) {
	if (!buffer.size() || buffer.size() > MAX_FRAME_SIZE) {
		closeSocket(CLOSE_TOO_LARGE, "Unexpected message size");
		return;
	}
}