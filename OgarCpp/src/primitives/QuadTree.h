#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <iostream>
#include <list>
#include <algorithm>
#include <string>
#include "Rect.h"

using std::function;
using std::atomic;

class QuadNode;
class QuadTree;

class QuadItem : public Point {
	friend QuadTree;
public:
	QuadNode* root;
	Rect range;
	QuadItem(const float x, const float y) : Point(x, y), root(nullptr) {};
};

class QuadTree {
	friend std::ostream& operator<<(std::ostream& stream, QuadTree& quad);
public:
	QuadNode* root;
	int maxLevel;
	int maxItem;
	unsigned long id = 0;
	atomic<unsigned int> reference = 0;
	QuadTree(Rect& range, int maxLevel, int maxItem, bool cleanup = false);
	~QuadTree();
	void insert(QuadItem*, bool nosplit = false);
	void split();
	void update(QuadItem*);
	void remove(QuadItem*);
	void search(Rect&, function<void(QuadItem*)> callback);
	bool containAny(Rect&, function<bool(QuadItem*)> selector);
};
