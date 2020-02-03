#pragma once

#include <uwebsockets/App.h>

#include "../ServerHandle.h"
class ServerHandle;

#include "../primitives/Logger.h"

struct SocketData {};

class Listener {
public:
	ServerHandle* handle;
	us_listen_socket_t* socketServer;
	thread* socketThread;
	// ChatChannel
	// Routers
	// Connections
	// ConnectionsByIP

	Listener(ServerHandle* handle) : handle(handle),
		socketServer(nullptr), socketThread(nullptr) {};
	~Listener() { close(); }

	bool open() {
		if (socketServer || socketThread) return false;
		int port = handle->getSettingInt("listeningPort");

		debug(string("listener opening at port ") + to_string(port));

		socketThread = new thread([this, port] {
			uWS::App().ws<SocketData>("/", {
				/* Settings */
				.compression = uWS::SHARED_COMPRESSOR,
				.maxPayloadLength = 16 * 1024,
				.maxBackpressure = 1 * 1024 * 1204,
				/* Handlers */
				.open = [](auto* ws, auto* req) {},
				.message = [](auto* ws, string_view message, uWS::OpCode opCode) {},
				.drain = [](auto* ws) { /* Check getBufferedAmount here */ },
				.ping  = [](auto* ws) {},
				.pong  = [](auto* ws) {},
				.close = [](auto* ws, int code, string_view message) {}
			}).listen(port, [this, port](auto* listenerSocket) {
				if (listenerSocket) {
					socketServer = listenerSocket;
					info(string("listener opened at port ") + to_string(port));
				} else {
					error(string("listener failed to open at port " + to_string(port)));
				}
			}).run();
		});
		socketThread->detach();
		return true;
	}

	bool close() {
		if (!socketServer && !socketThread) return false;
		us_listen_socket_close(0, socketServer);
		socketServer = nullptr;
		delete socketThread;
		socketThread = nullptr;
		debug("listener closed");
		return true;
	}
};