#pragma once

#include <iostream>

struct Boost {
	double dx;
	double dy;
	double d;
};

struct Quadrant {
	bool t, b, l, r;
	void print(std::ostream& stream) {
		stream << "Quadrant(";
		if (t) stream << "top ";
		if (b) stream << "bottom ";
		if (l) stream << "left ";
		if (r) stream << "right ";
		stream << ")";
	}
};

class Point {
public:
	double x, y;
	Point() : x(0), y(0) {};
	Point(double x, double y) : x(x), y(y) {};
};

class Rect : public Point {
	// friend ostream & operator<<(ostream& stream, Rect& rect);
public:
	double w, h;

	Rect() : Point(), w(0), h(0) {};
	Rect(double x, double y, double w, double h) : Point(x, y), w(w), h(h) {};
	
	bool intersects(const Rect& other) {
		return x - w <= other.x + other.w && \
		  	x + w >= other.x - other.w && \
			y - h <= other.y + other.h && \
			y + h >= other.y - other.h;
	}

	bool fullyIntersects(const Rect& other) {
		return x - w >= other.x + other.w && \
			x + w <= other.x - other.w && \
			y - h >= other.y + other.h && \
			y + h <= other.y - other.h;
	}

	Quadrant getQuadIntersect(const Rect& other) {
		return Quadrant{
			.t = y - h < other.y || y + h < other.y,
			.b = y - h > other.y || y + h > other.y,
			.l = x - w < other.x || x + w < other.x,
			.r = x - w > other.x || x + w > other.x,
		};
	}

	Quadrant getQuadFullIntersect(const Rect& other) {
		return Quadrant{
			.t = y - h < other.y && y + h < other.y,
			.b = y - h > other.y && y + h > other.y,
			.l = x - w > other.x && x + w < other.x,
			.r = x - w > other.x && x + w > other.x,
		};
	}

	void print(std::ostream& stream) {
		stream << "Rect { x: " << x << ", y: " << y << ", w: " << w << ", h: " << h << " }" << std::endl;
	}
};

class ViewArea : public Rect {
public:
	double s;
	ViewArea() : Rect(), s(0) {};
	ViewArea(double x, double y, double w, double h, double s) : Rect(x, y, w, y), s(s) {};

	void print(std::ostream& stream) {
		stream << "Rect { x: " << x << ", y: " << y << ", w: " << w << ", h: " << h << ", s:" << s << " }" << std::endl;
	}
};

/*
ostream& operator<<(ostream& stream, Rect& rect) {
	return stream << "[" << rect.x << "," << rect.y << "," << rect.w << "," << rect.h << "]";
}
*/