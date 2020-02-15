#include "../primitives/Logger.h"
#include "Listener.h"
#include "../ServerHandle.h"

/*
#include <regex>
static const std::regex IPv4MappedValidate{ "^::ffff:(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})$" };
std::string filterIPAddress(std::string ip) {
	std::smatch sm;
	std::regex_match(ip, sm, IPv4MappedValidate);
	if (sm.size()) {
		return sm[1];
	}
	return ip;
} */

bool Listener::open() {
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
			.open = [this](auto* ws, auto* req) {
				std::string_view ip_buff = ws->getRemoteAddress();
				unsigned int ipv4 = ip_buff.size() == 4 ? *((unsigned int*) ip_buff.data()) : 0;

				if (verifyClient(ipv4, req)) {
					onConnection(ipv4, ws);
					Logger::info("Connected");
				} else {
					ws->close();
					Logger::warn("Client verification failed");
				}
			},
			.message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {},
			.drain = [](auto* ws) { /* Check getBufferedAmount here */ },
			.ping = [](auto* ws) {},
			.pong = [](auto* ws) {},
			.close = [](auto* ws, int code, std::string_view message) {}
		}).listen("0.0.0.0", port, [this, port](auto* listenerSocket) {
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
};

bool Listener::close() {
	if (!socketServer && !socketThread) return false;
	us_listen_socket_close(0, socketServer);
	socketServer = nullptr;
	delete socketThread;
	socketThread = nullptr;
	Logger::debug("listener closed");
	return true;
};

bool Listener::verifyClient(unsigned int ipv4, uWS::HttpRequest* req) {

	if (!ipv4) return false;

	// Log header
	/*
	auto iter = req->begin();
	while (iter != req->end()) {
		std::cout << (*iter).first << ": " << (*iter).second << std::endl;
		++iter;
	} */

	// TODO check connection list length
	// TODO check request origin
	// TODO check IP black list (use kernal is probably better)

	// check connection per IP
	int ipLimit = handle->getSettingInt("listenerMaxConnectionsPerIP");
	if (ipLimit > 0 && connectionsByIP.contains(ipv4) && 
		connectionsByIP[ipv4] >= ipLimit) return false;

	return true;
}

void Listener::onConnection(unsigned int ipv4, uWS::WebSocket<false, true>* socket) {
	auto view = socket->getRemoteAddress();
	std::cout << "IP: ";
	for (auto c : view) {
		std::cout << std::to_string((int) c) << " ";
	}
	std::cout << std::endl;

	if (connectionsByIP.contains(ipv4)) {
		connectionsByIP[ipv4]++;
	} else {
		connectionsByIP.insert(std::make_pair(ipv4, 1));
	}
};

void Listener::onDisconnection(uWS::WebSocket<false, true>* socket, int code, std::string_view message) {

};

void Listener::update() {

};


