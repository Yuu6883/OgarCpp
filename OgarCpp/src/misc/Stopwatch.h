#pragma once

#include <chrono>

using namespace std::chrono;

class Stopwatch {
	time_point<steady_clock> _start;
	time_point<steady_clock> _lap;
public:
	Stopwatch() {};
	void begin() {
		reset();
	}
	double lap() {
		double v = duration<double, std::milli>(steady_clock::now() - _lap).count();
		_lap = steady_clock::now();
		return v;
	}
	double elapsed() {
		return duration<double, std::milli>(steady_clock::now() - _start).count();
	}
	void stop() {}
	void reset() {
		_start = _lap = steady_clock::now();
	}
};