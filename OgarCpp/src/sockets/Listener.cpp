#include "../primitives/Logger.h"
#include "Listener.h"
#include "Connection.h"
#include "ChatChannel.h"
#include "../ServerHandle.h"

enum CustomErrorCode : short {
	INVALID_IP = 4000,
	CONNECTION_MAXED,
	UNKNOWN_ORIGIN,
	IP_LIMITED
};

using std::string;
using std::to_string;

#define LOAD_SSL_OPTION(op) if (GAME_CONFIG[#op].is_string()) options.op = string(GAME_CONFIG[#op]).c_str();

bool Listener::open(int threads = 1) {
	if (threads < 1) {
		threads = 1;
		Logger::warn("Socket thread number is set to 1");
	} else if (threads > std::thread::hardware_concurrency()) {
		threads = std::thread::hardware_concurrency();
		Logger::warn(string("Socket thread number is set to ") + to_string(threads));
	}
	if (sockets.size() || socketThreads.size()) return false;

	originRegex = std::regex(handle->getSettingString("listenerAcceptedOriginRegex"));

	int port = handle->getSettingInt("listeningPort");

#if _WIN32
	threads = 1;
#endif

	if (!globalChat)
		globalChat = new ChatChannel(this);

	Logger::debug(std::to_string(threads) + " listener(s) opening at port " + std::to_string(port));

	bool ssl = false;
	
	if (GAME_CONFIG["enableSSL"].is_boolean())
		ssl = GAME_CONFIG["enableSSL"];

	for (int th = 0; th < threads; th++) {
		socketThreads.push_back(new std::thread([this, port, th, ssl] {
			if (ssl) {
				us_socket_context_options_t options;
				LOAD_SSL_OPTION(key_file_name);
				LOAD_SSL_OPTION(cert_file_name);
				LOAD_SSL_OPTION(passphrase);
				LOAD_SSL_OPTION(dh_params_file_name);
				LOAD_SSL_OPTION(ca_file_name);
				uWS::SSLApp(options).ws<SocketData>("/", {
					/* Settings */
					.compression = uWS::SHARED_COMPRESSOR,
					.maxPayloadLength = 16 * 1024,
					.maxBackpressure = 1 * 1024 * 1204,
					/* Handlers */
					.open = [this](auto* ws, auto* req) {
						if (handle->exiting) return;
						// req object gets yeet'd after return, capture origin to pass into loop::defer
						std::string origin = string(req->getHeader("origin"));
						auto data = (SocketData*)ws->getUserData();

						data->loop = uWS::Loop::get();
						data->loop->defer([this, data, ws, origin] {
							std::string_view ip_buff = ws->getRemoteAddress();
							unsigned int ipv4 = ip_buff.size() == 4 ? *((unsigned int*)ip_buff.data()) : 0;

							if (verifyClient(ipv4, ws, origin)) {
								data->connection = onConnection(ipv4, ws);
								Logger::info("Connected");
							} else {
							  Logger::warn("Client verification failed");
							}
						});
					},
					.message = [this](auto* ws, std::string_view buffer, uWS::OpCode opCode) {
						if (handle->exiting) return;
						auto data = (SocketData*)ws->getUserData();
						if (data->connection)
							data->connection->onSocketMessage(buffer);
					},
					.drain = [this](auto* ws) {
						if (handle->exiting) return;
						auto amount = ws->getBufferedAmount();
						if (!amount) {
							auto data = (SocketData*)ws->getUserData();
							if (data->connection) {
								data->connection->busy = false;
								if (data->connection->player)
									Logger::debug("Backpressure drained: " + data->connection->player->leaderboardName);
								}
							}
						else {
							Logger::warn("WebSocket still have backpressure after drain called: " + to_string(amount));
						}
					},
					.ping = [](auto* ws) {},
					.pong = [](auto* ws) {},
					.close = [this](auto* ws, int code, std::string_view message) {
						if (handle->exiting) return;
						auto data = (SocketData*)ws->getUserData();
						if (data->connection)
							data->connection->onSocketClose(code, message);
					}
				}).listen("0.0.0.0", port, [this, port, th](us_listen_socket_t* listenerSocket) {
					if (listenerSocket) {
						sockets.push_back(listenerSocket);
							Logger::info(string("listener#") + to_string(th) + string(" opened at port ") + to_string(port));
					} else {
						Logger::error(string("listener#") + to_string(th) + string(" failed to open at port ") + to_string(port));
					}
				}).run();

			} else {

				uWS::App().ws<SocketData>("/", {
					/* Settings */
					.compression = uWS::SHARED_COMPRESSOR,
					.maxPayloadLength = 16 * 1024,
					.maxBackpressure = 1 * 1024 * 1204,
					/* Handlers */
					.open = [this](auto* ws, auto* req) {
						if (handle->exiting) return;
						// req object gets yeet'd after return, capture origin to pass into loop::defer
						std::string origin = std::string(req->getHeader("origin"));
						auto data = (SocketData*)ws->getUserData();

						data->loop = uWS::Loop::get();
						data->loop->defer([this, data, ws, origin] {
							std::string_view ip_buff = ws->getRemoteAddress();
							unsigned int ipv4 = ip_buff.size() == 4 ? *((unsigned int*)ip_buff.data()) : 0;

							if (verifyClient(ipv4, ws, origin)) {
								data->connection = onConnection(ipv4, ws);
								Logger::info("Connected");
							} else {
							  Logger::warn("Client verification failed");
							}
						});
					},
					.message = [this](auto* ws, std::string_view buffer, uWS::OpCode opCode) {
						if (handle->exiting) return;
						auto data = (SocketData*)ws->getUserData();
						if (data->connection)
							data->connection->onSocketMessage(buffer);
					},
					.drain = [this](auto* ws) {
						if (handle->exiting) return;
						auto amount = ws->getBufferedAmount();
						if (!amount) {
							auto data = (SocketData*)ws->getUserData();
							if (data->connection) {
								data->connection->busy = false;
								if (data->connection->player)
									Logger::debug("Backpressure drained: " + data->connection->player->leaderboardName);
								}
							}
						else {
							Logger::warn("WebSocket still have backpressure after drain called: " + to_string(amount));
						}
					},
					.ping = [](auto* ws) {},
					.pong = [](auto* ws) {},
					.close = [this](auto* ws, int code, std::string_view message) {
						if (handle->exiting) return;
						auto data = (SocketData*)ws->getUserData();
						if (data->connection)
							data->connection->onSocketClose(code, message);
					}
				}).listen("0.0.0.0", port, [this, port, th](us_listen_socket_t* listenerSocket) {
					if (listenerSocket) {
						sockets.push_back(listenerSocket);
							Logger::info(string("listener#") + to_string(th) + string(" opened at port ") + to_string(port));
					} else {
						Logger::error(string("listener#") + to_string(th) + string(" failed to open at port ") + to_string(port));
					}
				}).run();
			}
		}));
	}
	return true;
};

bool Listener::close() {
	if (sockets.empty() && socketThreads.empty()) return false;
	for (auto socket : sockets)
		us_listen_socket_close(0, socket);

	sockets.clear();
	socketThreads.clear();

	if (globalChat) {
		delete globalChat;
		globalChat = nullptr;
	}

	Logger::debug("listener(s) closed");
	return true;
};

template<bool SSL>
bool Listener::verifyClient(unsigned int ipv4, uWS::WebSocket<SSL, true>* socket, std::string origin) {

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
	if (externalRouters.load() >= handle->runtime.listenerMaxConnections) {
		Logger::warn(string("CONNECTION MAXED: ") + to_string(handle->runtime.listenerMaxConnections));
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
	int ipLimit = handle->runtime.listenerMaxConnectionsPerIP;
	if (ipLimit > 0 && connectionsByIP.find(ipv4) != connectionsByIP.cend() &&
		connectionsByIP[ipv4] >= ipLimit) {
		socket->end(IP_LIMITED, "IP limited");
		return false;
	}

	return true;
}

unsigned long Listener::getTick() {
	return handle->tick;
}

// Called in socket thread
template<bool SSL>
Connection* Listener::onConnection(unsigned int ipv4, uWS::WebSocket<SSL, true>* socket) {
	auto connection = new Connection(this, ipv4, socket);
	if (connectionsByIP.find(ipv4) != connectionsByIP.cend()) {
		connectionsByIP[ipv4]++;
	} else {
		connectionsByIP.insert(std::make_pair(ipv4, 1));
	}
	externalRouters++;
	routers.push_back(connection);
	return connection;
};

void Listener::onDisconnection(Connection* connection, int code, std::string_view message) {
	Logger::debug(string("Socket closed { code: ") + to_string(code) + ", reason: " + string(message) + " }");
	if (--connectionsByIP[connection->ipv4] <= 0)
		connectionsByIP.erase(connection->ipv4);
	globalChat->remove(connection);
};

void Listener::update() {

	auto iter = routers.begin();
	while (iter != routers.cend()) {
		auto r = *iter;
		if (!r->shouldClose()) {
			iter++; 
			continue;
		}
		iter = routers.erase(iter);
		if (r->isExternal())
			externalRouters--;
		if (r->type == RouterType::PLAYER) {
			auto c = (Connection*) r;
			onDisconnection(c, c->closeCode, c->closeReason);
			if (c->socketDisconnected && !c->disconnected) {
				c->disconnected = true;
				c->disconnectedTick = handle->tick;
			}
		}
		r->close();
	}

	for (auto r : routers) r->update();
};


