#include "Player.h"
#include "../worlds/World.h"
#include "../ServerHandle.h"

Player::Player (ServerHandle* handle, unsigned int id, Router* router) :
	handle(handle), id(id), router(router) {
	viewArea.w /= handle->runtime.playerViewScaleMult;
	viewArea.h /= handle->runtime.playerViewScaleMult;
};

Player::~Player() {
	if (hasWorld) world->removePlayer(this);
	exists = false;
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

	double size = 0, x = 0, y = 0, score = 0;
	switch (state) {
		case PlayerState::DEAD:
			this->score = -1;
			break;
		case PlayerState::ALIVE:
			for (auto cell : ownedCells) {
				x += cell->x;
				y += cell->y;
				size += cell->size;
				score += cell->getMass();
			}
			viewArea.x = x / ownedCells.size();
			viewArea.y = y / ownedCells.size();
			this->score = score;
			size = viewArea.s = pow(std::min(64.0 / size, 1.0), 0.4);
			viewArea.w = 1920 / size / 2 * handle->runtime.playerViewScaleMult;
			viewArea.w = 1080 / size / 2 * handle->runtime.playerViewScaleMult;
			break;
		case PlayerState::SPEC:
			this->score = -1;
			viewArea = world->largestPlayer->viewArea;
			break;
		case PlayerState::ROAM:
			score = -1;
			double dx = router->mouseX - viewArea.x;
			double dy = router->mouseY - viewArea.y;
			double d = sqrt(dx * dx + dy * dy);
			double D = std::min(d, handle->runtime.playerRoamSpeed);
			if (D < 1) break;
			dx /= d; dy /= d;
			auto b = &world->border;
			viewArea.x = std::max(b->x - b->w, std::min(viewArea.x + dx * D, b->x + b->w));
			viewArea.y = std::max(b->y - b->h, std::min(viewArea.y + dy * D, b->y + b->h));
			size = viewArea.s = handle->runtime.playerRoamSpeed;
			viewArea.w = 1920 / size / 2 * handle->runtime.playerViewScaleMult;
			viewArea.w = 1080 / size / 2 * handle->runtime.playerViewScaleMult;
			break;
	}
}

void Player::updateVisibleCells() {
	if (!world) return;

	lastVisibleCells.clear();
	lastVisibleCells = visibleCells;
	visibleCells.clear();

	for (auto cell : ownedCells)
		visibleCells.insert(std::make_pair(cell->id, cell));

	world->finder->search(viewArea, [this](auto c) {
		auto cell = (Cell*) c;
		visibleCells.insert(std::make_pair(cell->id, cell));
	});
}

void Player::checkExistence() {
	if (!router->disconnected) return;
	if (state != PlayerState::ALIVE) {
		handle->removePlayer(this->id);
		return;
	}
	int delay = handle->runtime.worldPlayerDisposeDelay;
	if (delay > 0 && handle->tick - router->disconnectedTick >= delay)
		handle->removePlayer(this->id);
}