#pragma once

#include <uwebsockets/App.h>
#include <vector>
#include <map>

class ServerHandle;
class Router;

struct SocketData {};

class Listener {
public:
	ServerHandle* handle;
	us_listen_socket_t* socketServer;
	std::thread* socketThread;
	// ChatChannel
	std::vector<Router*> routers;
	std::map<unsigned int, unsigned int> connectionsByIP;

	Listener(ServerHandle* handle) : handle(handle),
		socketServer(nullptr), socketThread(nullptr) {};
	~Listener() { close(); }

	bool open();
	bool close();
	bool verifyClient(unsigned int ipv4, uWS::HttpRequest* req);

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

	void onConnection(unsigned int ipv4, uWS::WebSocket<false, true>* socket);
	void onDisconnection(uWS::WebSocket<false, true>* socket, int code, std::string_view message);
	void update();
};