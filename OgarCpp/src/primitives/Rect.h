#pragma once

#include <iostream>

struct Boost {
	float dx;
	float dy;
	float d;
};

typedef unsigned char Quadrant;

#define QUAD_T 0x1
#define QUAD_B 0x2
#define QUAD_L 0x4
#define QUAD_R 0x8

#define QUAD_TL 0x5
#define QUAD_TR 0x9
#define QUAD_BL 0x6
#define QUAD_BR 0xa

class Point {
protected:
	float x, y;
public:
	Point() : x(0), y(0) {};
	Point(float x, float y) : x(x), y(y) {};
	float getX() { return x; };
	float getY() { return y; };
	void setX(float x) { this->x = x; };
	void setY(float y) { this->y = y; };
};

class Rect : public Point {
	// friend ostream & operator<<(ostream& stream, Rect& rect);
public:
	float w, h;

	Rect() : Point(), w(0), h(0) {};
	Rect(float x, float y, float w, float h) : Point(x, y), w(w), h(h) {};
	
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
		return ((y - h < other.y || y + h < other.y) && QUAD_T) |
			   ((y - h > other.y || y + h > other.y) && QUAD_B) |
			   ((x - w < other.x || x + w < other.x) && QUAD_L) |
			   ((x - w > other.x || x + w > other.x) && QUAD_R);
	}

	Quadrant getQuadFullIntersect(const Rect& other) {
		return ((y - h < other.y && y + h < other.y) && QUAD_T) |
			   ((y - h > other.y && y + h > other.y) && QUAD_B) |
			   ((x - w < other.x && x + w < other.x) && QUAD_L) |
			   ((x - w > other.x && x + w > other.x) && QUAD_R);
	}

	void print(std::ostream& stream) {
		stream << "Rect { x: " << x << ", y: " << y << ", w: " << w << ", h: " << h << " }" << std::endl;
	}
};

class ViewArea : public Rect {
public:
	float s;
	ViewArea() : Rect(), s(0) {};
	ViewArea(float x, float y, float w, float h, float s) : Rect(x, y, w, h), s(s) {};

	void print(std::ostream& stream) {
		stream << "Rect { x: " << x << ", y: " << y << ", w: " << w << ", h: " << h << ", s:" << s << " }" << std::endl;
	}
};

/*
ostream& operator<<(ostream& stream, Rect& rect) {
	return stream << "[" << rect.x << "," << rect.y << "," << rect.w << "," << rect.h << "]";
}
*/