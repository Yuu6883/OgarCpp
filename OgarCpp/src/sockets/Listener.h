#pragma once

#include <uwebsockets/App.h>
#include <atomic>
#include <regex>
#include <vector>
#include <list>
#include <map>

using std::atomic;

class ChatChannel;
class ServerHandle;
class Router;
class Connection;

struct SocketData {
	uWS::Loop* loop;
	Connection* connection = nullptr;
};

class Listener {
public:
	std::regex originRegex = std::regex("");
	ServerHandle* handle;
	std::vector<us_listen_socket_t*> sockets;
	std::vector<std::thread*> socketThreads;
	
	ChatChannel* globalChat = nullptr;

	atomic<unsigned int> externalRouters = 0;
	std::list<Router*> routers;
	std::map<unsigned int, unsigned int> connectionsByIP;

	Listener(ServerHandle* handle) : handle(handle) {};
	~Listener() { close(); }

	bool open(int);
	bool close();
	template<bool SSL>
	bool verifyClient(unsigned int ipv4, uWS::WebSocket<SSL, true>* socket, std::string origin);

	unsigned long getTick();
	template<bool SSL>
	Connection* onConnection(unsigned int ipv4, uWS::WebSocket<SSL, true>* socket);
	void onDisconnection(Connection* connection, int code, std::string_view message);
	void update();
};