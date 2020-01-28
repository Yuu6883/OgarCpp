#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <functional>

using namespace std;
using namespace chrono;

typedef function<void(void)> Callback;

class Ticker {

	int step;
	int ticked = 0;
	bool running = false;
	time_point<steady_clock> _start;
	vector<Callback> callbacks;
	thread* th = nullptr;

public:
	Ticker(int step) : step(step) {};

	void add(Callback cb) {
		callbacks.push_back(cb);
	}

	void clear() {
		callbacks.clear();
	}

	void start() {
		if (running) throw runtime_error("The ticker has already started");
		running = true;
		_start = steady_clock::now();
		th = new thread([&]() { tick(); });
		th->detach();
	}

	void stop() {
		if (!running) throw runtime_error("The ticker hasn't started");
		running = false;
		if (th->joinable()) th->join();
		delete th;
		th = nullptr;
	}

	void tick() {
		while (running) {
			for_each(callbacks.begin(), callbacks.end(), [](Callback cb) {
				cb();
			});
			this_thread::sleep_until(_start + ticked++ * milliseconds{ step });
		}
	}
};