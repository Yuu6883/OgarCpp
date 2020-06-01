#include "QuadTree.h"

using std::list;

class QuadNode {
public:
	unsigned int& maxLevel;
	unsigned int& maxItem;
	unsigned int level;
	bool cleanup;

	Rect range;
	QuadNode* root;
	QuadNode* branches;
	list<QuadItem*> items;

	QuadNode(Rect range, unsigned int& maxLevel, unsigned int& maxItem, QuadNode* root, bool cleanup = false) :
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
		bool done = false;
		while (!done) {
			if (!quad->hasSplit()) break;
			unsigned char quadrant = item->range.getQuadFullIntersect(quad->range);
			switch (quadrant) {
				case QUAD_TL:
					quad = &branches[0]; break;
				case QUAD_TR:
					quad = &branches[1]; break;
				case QUAD_BL:
					quad = &branches[2]; break;
				case QUAD_BR:
					quad = &branches[3]; break;
				default:
					done = true;  break;
			}
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
		bool done = false;
		while (!done) {
			if (!newQuad->hasSplit()) break;
			unsigned char quadrant = item->range.getQuadFullIntersect(newQuad->range);
			switch (quadrant) {
			case QUAD_TL:
				newQuad = &branches[0]; break;
			case QUAD_TR:
				newQuad = &branches[1]; break;
			case QUAD_BL:
				newQuad = &branches[2]; break;
			case QUAD_BR:
				newQuad = &branches[3]; break;
			default:
				done = true;  break;
			}
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
			QuadNode(Rect(x - hw, y - hh, hw, hh), maxLevel, maxItem, this),
			QuadNode(Rect(x + hw, y - hh, hw, hh), maxLevel, maxItem, this),
			QuadNode(Rect(x - hw, y + hh, hw, hh), maxLevel, maxItem, this),
			QuadNode(Rect(x + hw, y + hh, hw, hh), maxLevel, maxItem, this),
		};
		auto iter = items.begin();
		while (iter != items.cend()) {
			unsigned char quadrant = (*iter)->range.getQuadFullIntersect(range);
			switch (quadrant) {
				case QUAD_TL:
					branches[0].insert(*iter); break;
				case QUAD_TR:
					branches[1].insert(*iter); break;
				case QUAD_BL:
					branches[2].insert(*iter); break;
				case QUAD_BR:
					branches[3].insert(*iter); break;
				default:
					iter++;
					continue;
			}
			iter = items.erase(iter);
		}
	};

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
	
	unsigned int searchDFS(Rect& r, function<void(QuadItem*)> callback) {
		unsigned int count = 0;
		for (auto item : items) {
			if (r.intersects(item->range)) {
				callback(item);
				count++;
			}
		}
		if (!hasSplit()) return count;
		auto quad = r.getQuadIntersect(range);
		switch (quad) {
			case QUAD_TL:
				count += branches[0].searchDFS(r, callback); break;
			case QUAD_TR:
				count += branches[1].searchDFS(r, callback); break;
			case QUAD_BL:
				count += branches[2].searchDFS(r, callback); break;
			case QUAD_BR:
				count += branches[3].searchDFS(r, callback);
			default:
				break;
		}
		return count;
	};

	unsigned int searchBFS(Rect& r, function<bool(QuadItem*)> callback) {
		unsigned int count = 0;
		for (auto item : items) {
			if (r.intersects(item->range)) {
				if (callback(item)) count++;
			}
		}
		return count;
	}

	
	bool containAny(Rect& r, function<bool(QuadItem*)> selector) {
		for (auto item : items)
			if (r.intersects(item->range) && selector(item)) return true;
	
		if (!hasSplit()) return false;
		auto quad = r.getQuadIntersect(range);
		switch (quad) {
			case QUAD_TL:
				if (branches[0].containAny(r, selector)) return true; break;
			case QUAD_TR:
				if (branches[1].containAny(r, selector)) return true; break;
			case QUAD_BL:
				if (branches[2].containAny(r, selector)) return true; break;
			case QUAD_BR:
				if (branches[3].containAny(r, selector)) return true; break;
			default:
				break;
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

QuadTree::QuadTree(Rect& range, unsigned int maxLevel, unsigned int maxItem, bool cleanup) :
	maxLevel(maxLevel), maxItem(maxItem) {
	root = new QuadNode(range, this->maxLevel, this->maxItem, nullptr, cleanup);

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
unsigned int QuadTree::search(Rect& rect, function<bool(QuadItem*)> callback) {
	unsigned int count = 0;
	if (root) {
		if (maxSearch) {
			list<QuadNode*> queue;
			queue.push_back(root);

			while (!queue.empty()) {
				auto node = queue.front();
				count += node->searchBFS(rect, callback);
				queue.pop_front();

				if (count >= maxSearch) {
					// printf("search capped: %u, max: %u\n", count, maxSearch);
					break;
				}

				if (node->hasSplit()) {
					queue.push_back(&node->branches[0]);
					queue.push_back(&node->branches[1]);
					queue.push_back(&node->branches[2]);
					queue.push_back(&node->branches[3]);
				}
			}
		} else {
			count = root->searchDFS(rect, callback);
		}
	}
	return count;
}
bool QuadTree::containAny(Rect& rect, function<bool(QuadItem*)> selector) {
	if (root) return root->containAny(rect, selector);
	return false;
};
