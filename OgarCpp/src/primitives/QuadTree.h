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
class QuadItem : public Point {
public:
	QuadNode* root;
	Rect range;
	QuadItem(const float x, const float y) : Point(x, y), root(nullptr) {};
};

class QuadTree {
	friend std::ostream& operator<<(std::ostream& stream, QuadTree& quad);
public:
	QuadNode* root;
	unsigned int maxLevel;
	unsigned int maxItem;
	unsigned int maxSearch = 0;
	atomic<unsigned int> reference = 0;
	QuadTree(Rect& range, unsigned int maxLevel, unsigned int maxItem, bool cleanup = false);
	~QuadTree();
	void insert(QuadItem*, bool nosplit = false);
	void split();
	void update(QuadItem*);
	void remove(QuadItem*);
	unsigned int search(Rect&, function<bool(QuadItem*)> callback);
	bool containAny(Rect&, function<bool(QuadItem*)> selector);
};

static std::list<QuadTree*> cleanupQueue;
static inline void FREE_QUADTREES() {
	while (cleanupQueue.size()) {
		delete cleanupQueue.front();
		cleanupQueue.pop_front();
	}
};
