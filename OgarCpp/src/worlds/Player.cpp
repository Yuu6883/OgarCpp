#include "Player.h"
#include "../worlds/World.h"
#include "../ServerHandle.h"

Player::Player(ServerHandle* handle, unsigned int id, Router* router) :
	handle(handle), id(id), router(router) {
	viewArea.w /= handle->runtime.playerViewScaleMult;
	viewArea.h /= handle->runtime.playerViewScaleMult;
};

Player::~Player() {
	if (hasWorld) {
		Logger::warn("Player should not have world reference when it's being deallocated");
	}
	if (router->disconnected) {
		router->hasPlayer = false;
		router->player = nullptr;
		delete (Connection*) router;
	}
};

void Player::updateState(PlayerState targetState) {
	if (!world) state = PlayerState::DEAD;
	else if (ownedCells.size()) state = PlayerState::ALIVE;
	else if (targetState == PlayerState::DEAD) state == PlayerState::DEAD;
	else if (!world->largestPlayer) state = PlayerState::ROAM;
	else if (state == PlayerState::SPEC && targetState == PlayerState::ROAM) state = PlayerState::ROAM;
	else state = PlayerState::SPEC;
};

void Player::updateViewArea() {

	if (!world) return;

	float size = 0, x = 0, y = 0, score = 0;
	switch (state) {
		case PlayerState::DEAD:
			this->score = -1;
			break;
		case PlayerState::ALIVE:
			for (auto cell : ownedCells) {
				x += cell->getX();
				y += cell->getY();
				size += cell->getSize();
				score += cell->getMass();
			}
			viewArea.setX(x / ownedCells.size());
			viewArea.setY(y / ownedCells.size());
			this->score = score;
			size = viewArea.s = pow(std::min(64.0 / size, 1.0), 0.4);
			viewArea.w = 1920 / size / 2 * handle->runtime.playerViewScaleMult;
			viewArea.h = 1080 / size / 2 * handle->runtime.playerViewScaleMult;
			break;
		case PlayerState::SPEC:
			this->score = -1;
			viewArea = world->largestPlayer->viewArea;
			break;
		case PlayerState::ROAM:
			score = -1;
			float dx = router->mouseX - viewArea.getX();
			float dy = router->mouseY - viewArea.getY();
			float d = sqrt(dx * dx + dy * dy);
			float D = std::min(d, handle->runtime.playerRoamSpeed);
			if (D < 1) break;
			dx /= d; dy /= d;
			auto b = &world->border;
			viewArea.setX(std::max(b->getX() - b->w, std::min(viewArea.getX() + dx * D, b->getX() + b->w)));
			viewArea.setY(std::max(b->getY() - b->h, std::min(viewArea.getY() + dy * D, b->getY() + b->h)));
			size = viewArea.s = handle->runtime.playerRoamSpeed;
			viewArea.w = 1920 / size / 2 * handle->runtime.playerViewScaleMult;
			viewArea.h = 1080 / size / 2 * handle->runtime.playerViewScaleMult;
			break;
	}
}

void Player::updateVisibleCells() {
	if (!world) return;

	lastVisibleCells.clear();
	lastVisibleCells = visibleCells;
	visibleCells.clear();

	for (auto cell : ownedCells)
		if (cell->getType() != CellType::EJECTED_CELL || cell->getAge() > 1)
			visibleCells.insert(std::make_pair(cell->id, cell));

	world->finder->search(viewArea, [this](auto c) {
		auto cell = (Cell*) c;
		if (cell->getType() != CellType::EJECTED_CELL || cell->getAge() > 1)
			visibleCells.insert(std::make_pair(cell->id, cell));
	});
}

bool Player::exist() {
	if (!router->disconnected) return true;
	if (state != PlayerState::ALIVE) {
		handle->removePlayer(this->id);
		return false;
	}
	int delay = handle->runtime.worldPlayerDisposeDelay;
	if (router->disconnectedTick && delay > 0 && handle->tick - router->disconnectedTick >= delay) {
		handle->removePlayer(this->id);
		return false;
	}
	return true;
}