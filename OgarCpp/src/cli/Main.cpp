#define _HAS_STD_BYTE 0
#define _HAS_STD_BOOLEAN 0

#include <iostream>

#include "../ServerHandle.h"
#include "../Settings.h"

int main() {

	Setting* settings = loadConfig();
	ServerHandle handle(settings);

	Router r(&handle.listener);

	r.spawningName = "<clown>world";
	r.onSpawnRequest();

	handle.start();
	std::this_thread::sleep_for(std::chrono::seconds{ 200 });
	handle.stop();

	return EXIT_SUCCESS;
}