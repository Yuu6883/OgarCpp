#include "PlayerBot.h"
#include "../worlds/World.h"
#include "../worlds/Player.h"
#include "../ServerHandle.h"

PlayerBot::PlayerBot(World* world) : Router(&world->handle->listener) {
	type = RouterType::PLAYER_BOT;
	listener->routers.push_back(this);
};

void PlayerBot::update() {
	if (splitCooldownTicks > 0) splitCooldownTicks--;
	else target = nullptr;

	player->updateVisibleCells();
	if (player->state != PlayerState::ALIVE) {
		spawningName = listener->handle->randomBotName();
		spawningSkin = listener->handle->randomBotSkin();
		requestSpawning = true;
	} else {
		PlayerCell* biggestCell = nullptr;
		for (auto cell : player->ownedCells)
			if (!biggestCell || cell->getSize() > biggestCell->getSize())
				biggestCell = cell;
		if (!biggestCell) return;

		if (target) {
			if (!target->exist || !canEat(biggestCell->getSize(), target->getSize()))
				target = nullptr;
			else {
				mouseX = target->getX();
				mouseY = target->getY();
				return;
			}
		}

		bool atMaxCells = player->ownedCells.size() >= listener->handle->runtime.playerMaxCells;
		bool willingToSplit = player->ownedCells.size() <= listener->handle->runtime.playerMaxCells / 10;
		unsigned int cellCount = player->visibleCells.size();

		float mx = 0.0f, my = 0.0f;
		Cell* bestPrey = nullptr;
		bool splitkillObstacleNearby = false;
		float truncatedInfluence = log10f(biggestCell->getSquareSize());

		for (auto [_, check] : player->visibleCells) {
			float dx = check->getX() - biggestCell->getX();
			float dy = check->getY() - biggestCell->getY();
			float splitDist = std::max(1.0f, sqrt(dx * dx + dy * dy));
			float d = std::max(1.0f, splitDist - biggestCell->getSize() - check->getSize());
			float influence = 0.0f;
			switch (check->getType()) {
				case CellType::PLAYER:
					if (!check->owner || check->owner == player) break;
					if (player->team > 0 && player->team == check->owner->team) break;
					if (canEat(biggestCell->getSize(), check->getSize())) {
						influence = truncatedInfluence;
						if (!canSplitKill(biggestCell->getSize(), check->getSize(), splitDist)) break;
						if (!bestPrey || check->getSize() > bestPrey->getSize())
							bestPrey = check;
					} else {
						influence = canEat(check->getSize(), biggestCell->getSize()) ? -truncatedInfluence * cellCount : -1;
						splitkillObstacleNearby = true;
					}
					break;

				case CellType::PELLET:
					influence = 1.0f;
					break;

				case CellType::VIRUS:
					if (atMaxCells) influence = truncatedInfluence;
					else if (canEat(biggestCell->getSize(), check->getSize())) {
						influence = -1 * cellCount;
						if (canSplitKill(biggestCell->getSize(), check->getSize(), splitDist))
							splitkillObstacleNearby = true;
					}
					break;
				case CellType::EJECTED_CELL:
					if (canEat(biggestCell->getSize(), check->getSize())) influence = truncatedInfluence * cellCount;
					break;
				case CellType::MOTHER_CELL:
					if (canEat(check->getSize(), biggestCell->getSize())) influence = -1;
					else if (canEat(biggestCell->getSize(), check->getSize())) {
						if (atMaxCells) influence = truncatedInfluence * cellCount;
						else influence = -1;
					}
					break;
			}

			if (!influence) continue;
			if (!d) d = 1.0f;
			dx /= d; dy /= d;
			mx += dx * influence / d;
			my += dy * influence / d;
		}

		if (willingToSplit && !splitkillObstacleNearby && splitCooldownTicks <= 0 &&
			bestPrey && bestPrey->getSize() * 2 > biggestCell->getSize()) {
			target = bestPrey;
			mouseX = bestPrey->getX();
			mouseY = bestPrey->getY();
			splitAttempts++;
			splitCooldownTicks = 25;
		} else {
			float d = std::max(1.0f, sqrt(mx * mx + my * my));
			mouseX = biggestCell->getX() + mx / d * player->viewArea.w;
			mouseY = biggestCell->getY() + my / d * player->viewArea.h;
		}
	}
}

bool PlayerBot::canEat(float aSize, float bSize) {
	return aSize > bSize * listener->handle->runtime.worldEatMult;
};

bool PlayerBot::canSplitKill(float aSize, float bSize, float d) {
	float splitDist = std::max(2 * aSize / listener->handle->runtime.playerSplitSizeDiv / 2,
		listener->handle->runtime.playerSplitBoost);
	return aSize / listener->handle->runtime.playerSplitSizeDiv > bSize * listener->handle->runtime.worldEatMult &&
		d - splitDist <= aSize - bSize / listener->handle->runtime.worldEatOverlapDiv;
};

void PlayerBot::close() {
	if (player->world)
		player->world->removePlayer(player);
	disconnected = true;
	disconnectedTick = listener->handle->tick;
}