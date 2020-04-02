#pragma once

#include <uwebsockets/App.h>
#include <regex>
#include <vector>
#include <map>
#include "ChatChannel.h"

class ChatChannel;
class ServerHandle;
class Router;
class Connection;

struct SocketData {
	uWS::Loop* loop;
	Connection* connection;
};

class Listener {
public:
	std::regex originRegex = std::regex("");
	ServerHandle* handle;
	std::vector<us_listen_socket_t*> sockets;
	std::vector<std::thread*> socketThreads;
	
	ChatChannel* globalChat;

	std::vector<Router*> routers;
	std::vector<Connection*> connections;
	std::map<unsigned int, unsigned int> connectionsByIP;

	Listener(ServerHandle* handle) : handle(handle) {};
	~Listener() { close(); }

	bool open(int);
	bool close();
	bool verifyClient(unsigned int ipv4, uWS::WebSocket<false, true>* socket, std::string origin);

	void addRouter(Router* router) { routers.push_back(router); };

	void removeRouter(Router* router) {
		auto iter = routers.begin();
		auto end  = routers.cend();
		while (iter != end) {
			if (*iter == router) {
				routers.erase(iter);
				return;
			}
			iter++;
		}
	}

	unsigned long getTick();
	Connection* onConnection(unsigned int ipv4, uWS::WebSocket<false, true>* socket);
	void onDisconnection(Connection* connection, int code, std::string_view message);
	void update();
};