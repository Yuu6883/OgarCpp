#pragma once

#include <string>
#include <functional>
#include "Rect.h"

using std::function;

class QuadNode;
class QuadItem : public Point {
public:
	QuadNode* root;
	Rect range;
	QuadItem(const double x, const double y) : Point(x, y), root(nullptr) {};
};

class QuadTree {
	friend std::ostream& operator<<(std::ostream& stream, QuadTree& quad);
private:
	QuadNode* root;
	int maxLevel;
	int maxItem;
public:
	QuadTree(Rect& range, int maxLevel, int maxItem);
	~QuadTree();
	void insert(QuadItem*);
	void update(QuadItem*);
	void remove(QuadItem*);
	void search(Rect&, function<void(QuadItem*)> callback);
	bool containAny(Rect&, function<bool(QuadItem*)> selector);
};
