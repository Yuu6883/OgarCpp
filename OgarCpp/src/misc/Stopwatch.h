#pragma once

#include <chrono>

using namespace std;
using namespace chrono;

class Stopwatch {
	time_point<steady_clock> _start;
	time_point<steady_clock> _lap;
public:
	Stopwatch() {};
	void begin() {
		reset();
	}
	double lap() {
		double v = duration<double, milli>(steady_clock::now() - _lap).count();
		_lap = steady_clock::now();
		return v;
	}
	double elapsed() {
		return duration<double, milli>(steady_clock::now() - _start).count();
	}
	void stop() {}
	void reset() {
		_start = _lap = steady_clock::now();
	}
};