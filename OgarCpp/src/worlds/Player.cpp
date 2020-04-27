#include "Player.h"
#include "../worlds/World.h"
#include "../ServerHandle.h"
#include "../bots/PlayerBot.h"

Player::Player(ServerHandle* handle, unsigned int id, Router* router) :
	handle(handle), id(id), router(router) {
	viewArea.w /= handle->runtime.playerViewScaleMult;
	viewArea.h /= handle->runtime.playerViewScaleMult;
};

Player::~Player() {
	Logger::debug(string("Deallocating player: ") + leaderboardName);
	if (hasWorld || world)
		Logger::warn("Player should NOT have reference to world while being deallocated");

	if (router->disconnected) {
		router->hasPlayer = false;
		router->player = nullptr;
		if (router->type == RouterType::PLAYER)
			delete (Connection*) router;
		else if (router->type == RouterType::PLAYER_BOT)
			delete (PlayerBot *) router;
		router = nullptr;
	}
};

void Player::updateState(PlayerState targetState) {
	if (!world) state = PlayerState::DEAD;
	else if (ownedCells.size()) state = PlayerState::ALIVE;
	else if (targetState == PlayerState::DEAD) {
		state = PlayerState::DEAD;
		router->onDead();
		killCount = 0;
		maxScore = 0;
	} else if (!world->largestPlayer) state = PlayerState::ROAM;
	else if (state == PlayerState::SPEC && targetState == PlayerState::ROAM) state = PlayerState::ROAM;
	else state = PlayerState::SPEC;
};

void Player::updateViewArea() {

	if (!world) return;

	float size = 0, size_x = 0, size_y = 0, x = 0, y = 0, score = 0, factor = 0;
	float min_x = world->border.getX() + world->border.w;
	float max_x = world->border.getX() - world->border.w;
	float min_y = world->border.getY() + world->border.h;
	float max_y = world->border.getY() - world->border.h;
	switch (state) {
		case PlayerState::DEAD:
			this->score = -1;
			break;
		case PlayerState::ALIVE:
			for (auto cell : ownedCells) {
				x += cell->getX() * cell->getSize();
				y += cell->getY() * cell->getSize();
				min_x = std::min(min_x, cell->getX());
				max_x = std::max(max_x, cell->getX());
				min_y = std::min(min_y, cell->getY());
				max_y = std::max(max_y, cell->getY());
				score += cell->getMass();
				size  += cell->getSize();
			}
			this->score = score;
			maxScore = std::max(score, maxScore);

			factor = pow(ownedCells.size() + 50, 0.1);
			viewArea.setX(x / size);
			viewArea.setY(y / size);
			size = (factor + 1) * sqrt(score * 100.0);
			size_x = size_y = std::max(size, 4000.0f);
			size_x = std::max(size_x, (viewArea.getX() - min_x) * 1.75f);
			size_x = std::max(size_x, (max_x - viewArea.getX()) * 1.75f);
			size_y = std::max(size_y, (viewArea.getY() - min_y) * 1.75f);
			size_y = std::max(size_y, (max_y - viewArea.getY()) * 1.75f);
			viewArea.w = size_x * handle->runtime.playerViewScaleMult;
			viewArea.h = size_y * handle->runtime.playerViewScaleMult;
			viewArea.s = size;
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

void Player::updateVisibleCells(bool threaded) {
	if (!hasWorld || !world) return;

	if (threaded && lockedFinder) {
		lastVisibleCellData.clear();
		lastVisibleCellData = visibleCellData;
		visibleCellData.clear();

		for (auto data : ownedCellData)
			if (data->type != CellType::EJECTED_CELL || data->age > 1)
				visibleCellData.insert(std::make_pair(data->id, data));

		// printf("Querying player#%u viewArea 0x%p\n", id, &viewArea);
		lockedFinder->search(viewArea, [this](auto c) {
			auto data = (CellData*)c;
			// if (data->type == CellType::EJECTED_CELL) printf("Cell#%u belongs to %u (%u) \n", data->id, data->pid, id);
			if (data->type != CellType::EJECTED_CELL || data->age > 1)
				visibleCellData.insert(std::make_pair(data->id, data));
		});

	} else {
		lastVisibleCells.clear();
		lastVisibleCells = visibleCells;
		visibleCells.clear();

		for (auto cell : ownedCells)
			if (cell->getType() != CellType::EJECTED_CELL || cell->getAge() > 1)
				visibleCells.insert(std::make_pair(cell->id, cell));

		world->finder->search(viewArea, [this](auto c) {
			auto cell = (Cell*)c;
			if (cell->getType() != CellType::EJECTED_CELL || cell->getAge() > 1)
				visibleCells.insert(std::make_pair(cell->id, cell));
		});

		/*
		printf("Visible Cells: ");
		for (auto [id, _] : visibleCells) {
			printf("%u, ", id);
		}
		printf("\n");

		printf("Lastvisible Cells: ");
		for (auto [id, _] : lastVisibleCells) {
			printf("%u, ", id);
		}
		printf("\n"); */
	}	
}

bool Player::exist() {
	if (!router->disconnected) return true;
	if (state != PlayerState::ALIVE) {
		world->removePlayer(this);
		handle->removePlayer(this->id);
		return false;
	}
	world->killPlayer(this);
	handle->removePlayer(this->id);
	return false;
}