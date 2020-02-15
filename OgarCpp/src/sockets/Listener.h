#pragma once

#include <uwebsockets/App.h>
#include <vector>

#include "../ServerHandle.h"
#include "./Router.h"
#include "../primitives/Logger.h"

class ServerHandle;
class Router;

struct SocketData {};

class Listener {
public:
	ServerHandle* handle;
	us_listen_socket_t* socketServer;
	std::thread* socketThread;
	// ChatChannel
	// Routers
	std::vector<Router*> routers;
	// Connections
	// ConnectionsByIP

	Listener(ServerHandle* handle) : handle(handle),
		socketServer(nullptr), socketThread(nullptr) {};
	~Listener() { close(); }

	bool open() {
		if (socketServer || socketThread) return false;
		int port = handle->getSettingInt("listeningPort");

		Logger::debug(std::string("listener opening at port ") + std::to_string(port));

		socketThread = new std::thread([this, port] {
			uWS::App().ws<SocketData>("/", {
				/* Settings */
				.compression = uWS::SHARED_COMPRESSOR,
				.maxPayloadLength = 16 * 1024,
				.maxBackpressure = 1 * 1024 * 1204,
				/* Handlers */
				.open = [](auto* ws, auto* req) {},
				.message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {},
				.drain = [](auto* ws) { /* Check getBufferedAmount here */ },
				.ping  = [](auto* ws) {},
				.pong  = [](auto* ws) {},
				.close = [](auto* ws, int code, std::string_view message) {}
			}).listen(port, [this, port](auto* listenerSocket) {
				if (listenerSocket) {
					socketServer = listenerSocket;
					Logger::info(std::string("listener opened at port ") + std::to_string(port));
				} else {
					Logger::error(std::string("listener failed to open at port " + std::to_string(port)));
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
		Logger::debug("listener closed");
		return true;
	}

	bool verifyClient() {

	}

	void addRouter(Router* router) {
		routers.push_back(router);
	}
};