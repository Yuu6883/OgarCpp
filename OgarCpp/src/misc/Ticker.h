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

	void clearInterval() {
		intervals.clear();
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
			for_each(callbacks.begin(), callbacks.end(), [](Callback cb) {
				cb();
			}); 
			for_each(intervals.begin(), intervals.end(), [this](pair<Callback, int> p) {
				if (!(ticked % p.second)) p.first();
			});
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