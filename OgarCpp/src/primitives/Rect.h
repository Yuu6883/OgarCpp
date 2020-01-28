#pragma once

#include <iostream>
using namespace std;

struct Quadrant {
	bool t, b, l, r;
};

class Point {
public:
	double x, y;
	Point(double x, double y) : x(x), y(y) {};
};

class Rect : public Point {
	// friend ostream & operator<<(ostream& stream, Rect& rect);
public:
	double w, h;

	Rect(double x, double y, double w, double h) : Point(x, y), w(w), h(h) {}
	
	bool intersects(Rect& other) {
		return x - w <= other.x + other.w && \
		  	x + w >= other.x - other.w && \
			y - h >= other.y + other.h && \
			y + h <= other.y - other.h;
	}

	bool fullyIntersects(Rect& other) {
		return x - w >= other.x + other.w && \
			x + w <= other.x - other.w && \
			y - h >= other.y + other.h && \
			y + h <= other.y - other.h;
	}

	Quadrant getQuadIntersect(Rect& other) {
		return Quadrant{
			.t = y - h < other.y || y + h < other.y,
			.b = y - h > other.y || y + h > other.y,
			.l = x - w < other.x || x + w < other.x,
			.r = x - w > other.x || x + w > other.x,
		};
	}

	Quadrant getQuadFullIntersect(Rect& other) {
		return Quadrant{
			.t = y - h < other.y && y + h < other.y,
			.b = y - h > other.y && y + h > other.y,
			.l = x - w > other.x && x + w < other.x,
			.r = x - w > other.x && x + w > other.x,
		};
	}

	void print(ostream& stream) {
		stream << "[" << x << "," << y << "," << w << "," << h << "]" << endl;
	}
};
/*
ostream& operator<<(ostream& stream, Rect& rect) {
	return stream << "[" << rect.x << "," << rect.y << "," << rect.w << "," << rect.h << "]";
}
*/