#include <chrono>
#include <thread>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <functional>

using namespace std::chrono;
using std::list;
using std::pair;
using std::make_pair;

typedef std::function<void(void)> Callback;

class Ticker {

	unsigned int step;
	unsigned long ticked = 0;
	time_point<steady_clock> _start;
	list<Callback> callbacks;
	list<pair<Callback, int>> intervals;
	list<pair<Callback, int>> timeouts;
	std::thread* th = nullptr;

public:

	bool running = false;
	Ticker() : step(40) {};
	Ticker(int step) : step(step) {};

	void setStep(unsigned int step) {
		this->step = step;
	}

	void add(Callback cb) {
		callbacks.push_back(cb);
	}

	void every(int interval, Callback cb) {
		intervals.push_back(make_pair(cb, interval));
	}

	void timeout(int timeout, Callback cb) {
		timeouts.push_back(make_pair(cb, timeout));
	}

	void clearInterval() {
		intervals.clear();
	}

	void clearTimeout() {
		timeouts.clear();
	}

	void clear() {
		callbacks.clear();
	}

	void start(bool join = false) {
		if (running) throw std::runtime_error("The ticker has already started");
		running = true;
		_start = steady_clock::now();
		th = new std::thread([&]() { tick(); });
		join ? th->join() : th->detach();
	}

	void stop() {
		if (!running) throw std::runtime_error("The ticker hasn't started");
		running = false;
		if (th->joinable()) th->join();
		delete th;
		th = nullptr;
	}

	void tick() {
		while (running) {
			for (auto cb : callbacks) cb();
			for (auto [cb, interval] : intervals) if (!(ticked % interval)) cb();
			auto iter = timeouts.begin();
			while (iter != timeouts.cend()) {
				auto [cb, timeout] = *iter;
				if (!timeout) {
					cb();
				} else {
					timeouts.push_front(make_pair(cb, timeout - 1));
				}
				iter = timeouts.erase(iter);
			}

			auto timepoint = _start + ticked++ * milliseconds{ step };
			auto delta = timepoint - steady_clock::now();
			if (delta.count() < 0) {
				_start -= delta;
				continue;
			}
			std::this_thread::sleep_until(timepoint);
		}
	}

	void join() {
		if (th && th->joinable())
			th->join();
	}
};