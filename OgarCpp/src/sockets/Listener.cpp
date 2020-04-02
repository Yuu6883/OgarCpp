#include "../primitives/Logger.h"
#include "Listener.h"
#include "Connection.h"
#include "../ServerHandle.h"

enum CustomErrorCode : short {
	INVALID_IP = 4000,
	CONNECTION_MAXED,
	UNKNOWN_ORIGIN,
	IP_LIMITED
};

using std::string;
using std::to_string;

bool Listener::open(int threads = 1) {
	if (sockets.size() || socketThreads.size()) return false;

	originRegex = std::regex(handle->getSettingString("listenerAcceptedOriginRegex"));

	int port = handle->getSettingInt("listeningPort");

#if _WIN32
	threads = 1;
#endif

	if (!globalChat) {
		globalChat = new ChatChannel(this);
	}

	Logger::debug(std::to_string(threads) + " listener(s) opening at port " + std::to_string(port));

	for (int th = 0; th < threads; th++) {
		socketThreads.push_back(new std::thread([this, port, th] {
			uWS::App().ws<SocketData>("/", {
				/* Settings */
				.compression = uWS::SHARED_COMPRESSOR,
				.maxPayloadLength = 16 * 1024,
				.maxBackpressure = 1 * 1024 * 1204,
				/* Handlers */
				.open = [this](auto* ws, auto* req) {
				    // req object gets yeet'd after return, capture origin to pass into loop::defer
					std::string origin = std::string(req->getHeader("origin"));
					auto data = (SocketData*) ws->getUserData();

					data->loop = uWS::Loop::get();
					data->loop->defer([this, data, ws, origin] {
						std::string_view ip_buff = ws->getRemoteAddress();
						unsigned int ipv4 = ip_buff.size() == 4 ? *((unsigned int*) ip_buff.data()) : 0;

						if (verifyClient(ipv4, ws, origin)) {
							data->connection = onConnection(ipv4, ws);
							Logger::info("Connected");
						} else {
							Logger::warn("Client verification failed");
						}
					});
				},
				.message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
				},
				.drain = [](auto* ws) { /* Check getBufferedAmount here */ },
				.ping = [](auto* ws) {},
				.pong = [](auto* ws) {},
				.close = [this](auto* ws, int code, std::string_view message) {
					auto data = (SocketData*)ws->getUserData();
					onDisconnection(data->connection, code, message);
				}
			}).listen("0.0.0.0", port, [this, port, th](us_listen_socket_t* listenerSocket) {
				if (listenerSocket) {
					sockets.push_back(listenerSocket);
					Logger::info(string("listener#") + to_string(th) + string(" opened at port ") + to_string(port));
				} else {
					Logger::error(string("listener#") + to_string(th) + string(" failed to open at port ") + to_string(port));
				}
			}).run();
		}));
	}
	return true;
};

bool Listener::close() {
	if (sockets.empty() && socketThreads.empty()) return false;
	for (auto socket : sockets) {
		us_listen_socket_close(0, socket);
	}
	sockets.clear();
	for (auto thread : socketThreads) {
		thread->join();
	}

	if (globalChat) {
		delete globalChat;
		globalChat = nullptr;
	}

	socketThreads.clear();
	Logger::debug("listener(s) closed");
	return true;
};

bool Listener::verifyClient(unsigned int ipv4, uWS::WebSocket<false, true>* socket, std::string origin) {

	if (!ipv4) {
		Logger::warn("INVALID IP");
		socket->end(INVALID_IP, "Invalid IP");
		return false;
	}

	// Log header
	/*
	auto iter = req->begin();
	while (iter != req->end()) {
		std::cout << (*iter).first << ": " << (*iter).second << std::endl;
		++iter;
	} */

	// check connection list length
	if (connections.size() >= handle->getSettingInt("listenerMaxConnections")) {
		Logger::warn("CONNECTION MAXED");
		socket->end(CONNECTION_MAXED, "Server max connection reached");
		return false;
	}

	// check request origin
	Logger::debug(std::string("Origin: ") + origin);
	if (!std::regex_match(std::string(origin), originRegex)) {
		socket->end(UNKNOWN_ORIGIN, "Unknown origin");
		return false;
	}

	// Maybe check IP black list (use kernal is probably better)

	// check connection per IP
	int ipLimit = handle->getSettingInt("listenerMaxConnectionsPerIP");
	if (ipLimit > 0 && connectionsByIP.contains(ipv4) &&
		connectionsByIP[ipv4] >= ipLimit) {
		socket->end(IP_LIMITED, "IP limited");
		return false;
	}

	return true;
}

unsigned long Listener::getTick() {
	return handle->tick;
}

Connection* Listener::onConnection(unsigned int ipv4, uWS::WebSocket<false, true>* socket) {
	auto connection = new Connection(this, ipv4, socket);
	if (connectionsByIP.contains(ipv4)) {
		connectionsByIP[ipv4]++;
	} else {
		connectionsByIP.insert(std::make_pair(ipv4, 1));
	}
	connections.push_back(connection);
	return connection;
};

void Listener::onDisconnection(Connection* connection, int code, std::string_view message) {
	Logger::debug(string("Socket closed { code: ") + to_string(code) + ", reason: " + string(message) + " }");
	if (--connectionsByIP[connection->ipv4] <= 0)
		connectionsByIP.erase(connection->ipv4);
	globalChat->remove(connection);
	auto iter = connections.begin();
	auto cend = connections.cend();
	while (iter != cend) {
		if (*iter == connection) {
			connections.erase(iter);
			return;
		}
		iter++;
	}
};

void Listener::update() {
	int i, l;
	for (i = 0, l = routers.size(); i < l; i++) {
		auto r = routers[i];
		if (!r->shouldClose()) continue;
		r->close(); i--; l--;
	}
	for (auto r : routers) r->update();
};


