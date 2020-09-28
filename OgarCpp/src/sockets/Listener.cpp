#include "../primitives/Logger.h"
#include "Listener.h"
#include "Connection.h"
#include "ChatChannel.h"
#include "../ServerHandle.h"

#include "../web/AsyncFileReader.h"
#include "../web/AsyncFileStreamer.h"
#include "../web/Middleware.h"

enum CustomErrorCode : short {
	INVALID_IP = 4000,
	CONNECTION_MAXED,
	UNKNOWN_ORIGIN,
	IP_LIMITED
};

using std::string;
using std::to_string;

#define LOAD_SSL_OPTION(op) if (GAME_CONFIG[#op].is_string()) options.op = string(GAME_CONFIG[#op]).c_str();

Listener::Listener(ServerHandle* handle) : handle(handle) {
	socketsPool = new ThreadPool(handle->getSettingInt("socketsThreads"));
};

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
	
	if (GAME_CONFIG["enableSSL"].is_boolean())
		ssl = GAME_CONFIG["enableSSL"];

	bool serveWeb = false;
	if (GAME_CONFIG["serveWeb"].is_boolean())
		serveWeb = GAME_CONFIG["serveWeb"];
	short webPort = ssl ? 443 : 80;
	if (GAME_CONFIG["webPort"].is_number_integer())
		webPort = GAME_CONFIG["webPort"];

	string webRoot = "";
	if (GAME_CONFIG["webRoot"].is_string())
		webRoot = GAME_CONFIG["webRoot"];

	us_socket_context_options_t options;
	LOAD_SSL_OPTION(key_file_name);
	LOAD_SSL_OPTION(cert_file_name);
	LOAD_SSL_OPTION(passphrase);
	LOAD_SSL_OPTION(dh_params_file_name);
	LOAD_SSL_OPTION(ca_file_name);

	if (serveWeb && webRoot.length()) {
		{
			std::mutex m;
			std::condition_variable cv;

			for (int th = 0; th < threads; th++) {
				socketThreads.push_back(new std::thread([this, options, webPort, webRoot, th, &m, &cv] {
					AsyncFileStreamer asyncFileStreamer(webRoot);
					if (ssl) {
						uWS::SSLApp(options).get("/*", [&asyncFileStreamer](auto* res, auto* req) {
							serveFile(res, req);
							asyncFileStreamer.streamFile(res, req->getUrl());
						}).listen(webPort, [this, webPort, th, &m, &cv](auto* token) {
							if (token) {
								std::lock_guard<std::mutex> lk(m);
								webservers.push_back(token);
								cv.notify_one();
							}
							else {
								Logger::error(string("WebServer(SSL) failed to open at port ") + to_string(webPort));
								std::terminate();
							}
						}).run();
					}
					else {
						uWS::App().get("/*", [&asyncFileStreamer](auto* res, auto* req) {
							serveFile(res, req);
							asyncFileStreamer.streamFile(res, req->getUrl());
						}).listen(webPort, [this, webPort, th, &m, &cv](auto* token) {
							if (token) {
								std::lock_guard<std::mutex> lk(m);
								webservers.push_back(token);
								cv.notify_one();
							}
							else {
								Logger::error(string("WebServer failed to open at port ") + to_string(webPort));
								std::terminate();
							}
						}).run();
					}
				}));
			}

			{
				std::unique_lock<std::mutex> lk(m);
				cv.wait(lk, [&threads, this] { return threads == webservers.size(); });
				Logger::info(to_string(threads) + " WebServer" + (threads > 1 ? "s" : "") +
					(ssl ? "(SSL)" : "") + " opened at port " + to_string(webPort));
			}
		}
	}

	{
		std::mutex m;
		std::condition_variable cv;

		for (int th = 0; th < threads; th++) {
			socketThreads.push_back(new std::thread([this, port, th, options, &m, &cv] {
				if (ssl) {
					uWS::SSLApp(options).ws<SocketData>("/", {
						/* Settings */
						.compression = uWS::SHARED_COMPRESSOR,
						.maxPayloadLength = 16 * 1024,
						.maxBackpressure = 1 * 1024 * 1204,
						/* Handlers */
						.upgrade = [this](auto* res, auto* req, auto* context) {

							/* You may read from req only here, and COPY whatever you need into your PerSocketData.
							 * PerSocketData is valid from .open to .close event, accessed with ws->getUserData().
							 * HttpRequest (req) is ONLY valid in this very callback, so any data you will need later
							 * has to be COPIED into PerSocketData here. */
							if (verifyClient(req, res)) {
								/* Immediately upgrading without doing anything "async" before, is simple */
								res->template upgrade<SocketData>({
									/* We initialize PerSocketData struct here */
									}, req->getHeader("sec-websocket-key"),
									req->getHeader("sec-websocket-protocol"),
									req->getHeader("sec-websocket-extensions"),
									context);
							}

							 /* If you don't want to upgrade you can instead respond with custom HTTP here,
							  * such as res->writeStatus(...)->writeHeader(...)->end(...); or similar.*/
						 },
						.open = [this](auto* ws) {
							if (handle->exiting) return;

							auto data = (SocketData*)ws->getUserData();

							auto loop = uWS::Loop::get();
							std::string ip = std::string(ws->getRemoteAddressAsText());
							data->connection = onConnection(ip, ws);
							data->connection->loop = loop;
							Logger::info("Connected");
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
							} else {
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
					}).listen("0.0.0.0", port, [this, port, th, &m, &cv](us_listen_socket_t* listenerSocket) {
						if (listenerSocket) {
							std::lock_guard<std::mutex> lk(m);
							sockets.push_back(listenerSocket);
						} else {
							Logger::error(string("SocketServer(SSL) failed to open at port ") + to_string(port));
							std::terminate();
						}
						cv.notify_one();
					}).run();

				} else {

					uWS::App().ws<SocketData>("/", {
						/* Settings */
						.compression = uWS::SHARED_COMPRESSOR,
						.maxPayloadLength = 16 * 1024,
						.maxBackpressure = 1 * 1024 * 1204,
						.upgrade = [this](auto* res, auto* req, auto* context) {
							/* You may read from req only here, and COPY whatever you need into your PerSocketData.
							 * PerSocketData is valid from .open to .close event, accessed with ws->getUserData().
							 * HttpRequest (req) is ONLY valid in this very callback, so any data you will need later
							 * has to be COPIED into PerSocketData here. */
							if (verifyClient(req, res)) {
								/* Immediately upgrading without doing anything "async" before, is simple */
								res->template upgrade<SocketData>({
									/* We initialize PerSocketData struct here */
									}, req->getHeader("sec-websocket-key"),
									req->getHeader("sec-websocket-protocol"),
									req->getHeader("sec-websocket-extensions"),
									context);
							}

							/* If you don't want to upgrade you can instead respond with custom HTTP here,
							 * such as res->writeStatus(...)->writeHeader(...)->end(...); or similar.*/
						 },
						/* Handlers */
						.open = [this](auto* ws) {
							if (handle->exiting) return;

							auto data = (SocketData*)ws->getUserData();

							auto loop = uWS::Loop::get();
							std::string ip = std::string(ws->getRemoteAddressAsText());
							data->connection = onConnection(ip, ws);
							data->connection->loop = loop;
							Logger::info("Connected");
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
					}).listen("0.0.0.0", port, [this, port, th, &m, &cv](us_listen_socket_t* listenerSocket) {
						if (listenerSocket) {
							std::lock_guard<std::mutex> lk(m);
							sockets.push_back(listenerSocket);
						} else {
							Logger::error(string("SocketServer failed to open at port ") + to_string(port));
						}
						cv.notify_one();
					}).run();
				}
			}));
		}

		{
			std::unique_lock<std::mutex> lk(m);
			cv.wait(lk, [&threads, this] { return threads == sockets.size(); });
			Logger::info(to_string(threads) + " SocketServer" + (threads > 1 ? "s" : "") +
				(ssl ? "(SSL)" : "") + " opened at port " + to_string(port));
		}
	}

	return true;
};

bool Listener::close() {
	if (sockets.empty() && socketThreads.empty()) return false;

	for (auto router : routers) router->close();

	if (externalRouters) {
		Logger::debug("Waiting for " + std::to_string(externalRouters.load()) + " connections to close");
		std::this_thread::sleep_for(std::chrono::milliseconds{ 500 }); // Dumb
		Logger::info("All external connections closed");
	}

	for (auto server : webservers)
		us_listen_socket_close(0, server);

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

bool Listener::verifyClient(uWS::HttpRequest* req, void* res) {

	uWS::HttpResponse<false>* http_res = nullptr;
	uWS::HttpResponse<true>* https_res = nullptr;

	if (ssl)
		https_res = (uWS::HttpResponse<true>*) res;
	else
		http_res = (uWS::HttpResponse<false>*) res;

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
		ssl ? https_res->writeStatus("400")->end("Server max connection reached") : 
			http_res->writeStatus("400")->end("Server max connection reached");
		return false;
	}

	string_view origin = req->getHeader("origin");

	// check request origin
	Logger::debug(std::string("Origin: ") + std::string(origin));
	if (!std::regex_match(std::string(origin), originRegex)) {
		ssl ? https_res->writeStatus("400")->end("Unknown origin") :
			http_res->writeStatus("400")->end("Unknown origin");
		return false;
	}

	// Maybe check IP black list (use kernal is probably better)

	// check connection per IP
	std::string ip = std::string(ssl ? https_res->getRemoteAddressAsText() : http_res->getRemoteAddressAsText());

	int ipLimit = handle->runtime.listenerMaxConnectionsPerIP;
	if (ipLimit > 0 && connectionsByIP.find(ip) != connectionsByIP.cend() &&
		connectionsByIP[ip] >= ipLimit) {
		ssl ? https_res->writeStatus("400")->end("IP limited") :
			http_res->writeStatus("400")->end("IP limited");
		return false;
	}

	return true;
}

unsigned long Listener::getTick() {
	return handle->tick;
}

// Called in socket thread=
Connection* Listener::onConnection(string ip, void* socket) {
    
    uWS::WebSocket<true, true>* s1 = nullptr;
    uWS::WebSocket<false, true>* s2 = nullptr;
    if (ssl)
        s1 = (uWS::WebSocket<true, true>*) socket;
    else
        s2 = (uWS::WebSocket<false, true>*) socket;
    
	auto connection = ssl ? new Connection(this, ip, s1) : new Connection(this, ip, s2);
	if (connectionsByIP.find(ip) != connectionsByIP.cend()) {
		connectionsByIP[ip]++;
	} else {
		connectionsByIP.insert(std::make_pair(ip, 1));
	}
	externalRouters++;
	routers.push_back(connection);
	return connection;
};

void Listener::onDisconnection(Connection* connection, int code, std::string_view message) {
	Logger::debug(string("Socket closed { code: ") + to_string(code) + ", reason: " + string(message) + " }");
	if (--connectionsByIP[connection->ip] <= 0)
		connectionsByIP.erase(connection->ip);
	externalRouters--;
	routers.remove(connection);
	globalChat->remove(connection);
	if (connection->player && connection->player->world)
		connection->player->world->worldChat->remove(connection);
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

	Stopwatch watch;
	watch.begin();
	for (auto r : routers) socketsPool->enqueue([r]() { r->update(); });
	socketsPool->waitFinished();
	for (auto r : routers) r->postUpdate();
	if (handle->bench) 
		printf("Routers update time: %f\n", watch.lap());
};


