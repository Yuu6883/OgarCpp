#pragma once
#pragma warning(disable : 4996)

#include <uwebsockets/App.h>
#include <atomic>
#include <regex>
#include <vector>
#include <list>
#include <map>
#include "../primitives/SimplePool.h"

using std::string;
using std::atomic;

class ChatChannel;
class ServerHandle;
class Router;
class Connection;

struct SocketData {
	Connection* connection = nullptr;
};

class Listener {
public:
    bool ssl = false;
	std::regex originRegex = std::regex("");
	ServerHandle* handle;
	std::vector<us_listen_socket_t*> webservers;
	std::vector<us_listen_socket_t*> sockets;
	std::vector<std::thread*> socketThreads;
	ThreadPool* socketsPool;
	
	ChatChannel* globalChat = nullptr;

	atomic<unsigned int> externalRouters = 0;
	std::list<Router*> routers;
	std::map<string, unsigned int> connectionsByIP;

	Listener(ServerHandle* handle);
	~Listener() {
		delete socketsPool;
		close();
	}

	bool open(int);
	bool close();
	bool verifyClient(uWS::HttpRequest* req, void* res);

	unsigned long getTick();
	Connection* onConnection(string ip, void* socket);
	void onDisconnection(Connection* connection, int code, std::string_view message);
	void update();
};