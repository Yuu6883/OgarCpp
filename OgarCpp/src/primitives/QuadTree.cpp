#include "QuadTree.h"

class QuadNode {
public:
	int& maxLevel;
	int& maxItem;
	int level;
	bool cleanup;

	Rect range;
	QuadNode* root;
	QuadNode* branches;
	std::list<QuadItem*> items;

	QuadNode(Rect range, int& maxLevel, int& maxItem, QuadNode* root, bool cleanup = false) :
		maxLevel(maxLevel), maxItem(maxItem), cleanup(cleanup), range(range), root(root), branches(nullptr) {
		level = root ? root->level + 1 : 1;
	}

	~QuadNode() {
		if (cleanup)
			for (auto item : items)
				delete item;
		if (!hasSplit()) return;
		delete[] branches;
		branches = nullptr;
	}

	bool hasSplit() {
		return branches != nullptr;
	}

	void insert(QuadItem* item) {
		auto quad = this;
		while (true) {
			if (!quad->hasSplit()) break;
			int quadrant = quad->getQuadrant(item->range);
			if (quadrant == -1) break;
			quad = &quad->branches[quadrant];
		}
		item->root = quad;
		quad->items.push_back(item);
		quad->split();
	};

	void update(QuadItem* item) {
		if (!item->root) return;
		auto oldQuad = item->root;
		auto newQuad = item->root;
		while (true) {
			if (!newQuad->root) break;
			newQuad = newQuad->root;
			if (newQuad->range.fullyIntersects(item->range)) break;
		}
		while (true) {
			if (!newQuad->hasSplit()) break;
			int quadrant = newQuad->getQuadrant(item->range);
			if (quadrant == -1) break;
			newQuad = &newQuad->branches[quadrant];
		}
		if (oldQuad == newQuad) return;

		oldQuad->items.remove(item);
		newQuad->items.push_back(item);
		item->root = newQuad;
		oldQuad->merge();
		newQuad->split();
	};

	void remove(QuadItem* item) {
		auto quad = item->root;
		quad->items.remove(item);
		item->root = nullptr;
		quad->merge();
	};

	void split() {
		if (hasSplit() || (level > maxLevel) || (items.size() < maxItem)) return;
		float x = range.getX();
		float y = range.getY();
		float hw = range.w / 2;
		float hh = range.h / 2;
		branches = new QuadNode[4]{
			QuadNode(Rect(x - hw, y - hh, hw, hh), maxLevel, maxItem, this, cleanup),
			QuadNode(Rect(x + hw, y - hh, hw, hh), maxLevel, maxItem, this, cleanup),
			QuadNode(Rect(x - hw, y + hh, hw, hh), maxLevel, maxItem, this, cleanup),
			QuadNode(Rect(x + hw, y + hh, hw, hh), maxLevel, maxItem, this, cleanup),
		};
		auto iter = items.begin();
		while (iter != items.cend()) {
			int quadrant = getQuadrant((*iter)->range);
			if (quadrant == -1) {
				iter++;
				continue;
			}
			branches[quadrant].insert(*iter);
			iter = items.erase(iter);
		}
	}

	void merge() {
		auto quad = this;
		while (quad) {
			if (!quad->hasSplit()) {
				quad = quad->root;
				continue;
			}
			for (int i = 0; i < 4; i++)
				if (quad->branches[i].hasSplit() || quad->branches[i].items.size() > 0)
					return;
			delete[] quad->branches;
			quad->branches = nullptr;
		}
	}

	unsigned int getItemCount() {
		if (!hasSplit()) return items.size();
		return items.size() +
			branches[0].getItemCount() + \
			branches[1].getItemCount() + \
			branches[2].getItemCount() + \
			branches[3].getItemCount();
	};

	int getBranchCount() {
		if (hasSplit()) {
			return 1 + \
				branches[0].getBranchCount() + branches[1].getBranchCount() + \
				branches[2].getBranchCount() + branches[3].getBranchCount();
		}
		return 1;
	};

	int getQuadrant(Rect& r) {
		auto quad = r.getQuadFullIntersect(range);
		if (quad.t) {
			if (quad.l) return 0;
			if (quad.r) return 1;
		}
		if (quad.b) {
			if (quad.l) return 2;
			if (quad.r) return 3;
		}
		return -1;
	};

	void search(Rect& r, function<void(QuadItem*)> callback) {
		for (auto item : items) {
			if (r.intersects(item->range))
				callback(item);
		}
		if (!hasSplit()) return;
		auto quad = r.getQuadIntersect(range);
		if (quad.t) {
			if (quad.l) branches[0].search(r, callback);
			if (quad.r) branches[1].search(r, callback);
		}
		if (quad.b) {
			if (quad.l) branches[2].search(r, callback);
			if (quad.r) branches[3].search(r, callback);
		}
	};

	bool containAny(Rect& r, function<bool(QuadItem*)> selector) {
		for (auto item : items) {
			if (r.intersects(item->range) && (!selector || selector(item)))
				return true;
		}
		if (!hasSplit()) return false;
		auto quad = r.getQuadIntersect(range);
		if (quad.t) {
			if (quad.l && branches[0].containAny(r, selector)) return true;
			if (quad.r && branches[1].containAny(r, selector)) return true;
		}
		if (quad.b) {
			if (quad.l && branches[2].containAny(r, selector)) return true;
			if (quad.r && branches[3].containAny(r, selector)) return true;
		}
		return false;
	};

};

std::ostream& operator<<(std::ostream& stream, QuadNode& quad) {
	stream << "items " << quad.items.size() << "/" << quad.maxItem << "/" << \
		quad.getItemCount() << " level " << quad.level << " x " << quad.range.getX() << " y " << quad.range.getY() << \
		" w " << quad.range.w << " h " << quad.range.h << std::endl;
	if (quad.hasSplit()) {
		for (int i = 0; i < 4; i++)
			stream << std::string(quad.level * 2, ' ') << quad.branches[i];
	}
	return stream;
}

std::ostream& operator<<(std::ostream& stream, QuadTree& tree) {
	if (tree.root) {
		return stream << *tree.root;
	}
	else {
		return stream << "[ROOTLESS TREE]" << std::endl;
	}
}

static unsigned long quadtree_id = 1;

QuadTree::QuadTree(Rect& range, int maxLevel, int maxItem, bool cleanup) :
	maxLevel(maxLevel), maxItem(maxItem) {
	root = new QuadNode(range, this->maxLevel, this->maxItem, nullptr, cleanup);
	id = quadtree_id++;

	if (cleanup) {
		if (cleanupQueue.size() > 5) {
			delete cleanupQueue.front();
			cleanupQueue.pop_front();
		}
		cleanupQueue.push_back(this);
	}
}

QuadTree::~QuadTree() {
	if (root) delete root;
}

void QuadTree::insert(QuadItem* item, bool nosplit) { 
	if (root) nosplit ? root->items.push_back(item) : root->insert(item);
};
void QuadTree::split() { if (root) root->split(); };
void QuadTree::update(QuadItem* item) { if (root) root->update(item); };
void QuadTree::remove(QuadItem* item) { if (root) root->remove(item); };
void QuadTree::search(Rect& rect, function<void(QuadItem*)> callback) {
	if (root) root->search(rect, callback);
}
bool QuadTree::containAny(Rect& rect, function<bool(QuadItem*)> selector) {
	if (root) return root->containAny(rect, selector);
	return false;
};
