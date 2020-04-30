#include "PlayerBot.h"
#include "../worlds/World.h"
#include "../worlds/Player.h"
#include "../ServerHandle.h"

PlayerBot::PlayerBot(World* world) : Router(&world->handle->listener) {
	type = RouterType::PLAYER_BOT;
	listener->routers.push_back(this);
	if (randomZeroToOne > 0.2f) selfeed = true;
	if (randomZeroToOne > 0.2f) trypopsplit = true;
	if (randomZeroToOne > 0.6f) revpopsplit = true;
	lockTicks = randomZeroToOne * 10;
	onDead();
};

unsigned int virusInRange(list<Cell*>& viruses, Cell* cell, float feedMass) {
	float radiusSquared = (cell->getMass() + feedMass) * 100;
	unsigned int count = 0;
	for (auto virus : viruses) {
		float dx = virus->getX() - cell->getX();
		float dy = virus->getY() - cell->getY();
		float dSquared = dx * dx + dy * dy;
		if (dSquared < radiusSquared) count++;
	}
	return count;
}

void PlayerBot::update() {
	if (player->world->stats.loadTime > 50.0f) {
		lockTicks = 5;
	}
	if (splitCooldownTicks > 0) splitCooldownTicks--;
	else target = nullptr;

	if (lockTicks > 0) {
		lockTicks--;
		return;
	}
	ejectMacro = false;

	player->updateVisibleCells();
	if (player->state != PlayerState::ALIVE && !requestSpawning) {
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
		bool willingToSplit = player->ownedCells.size() <= pow(listener->handle->runtime.playerMaxCells, 0.4f);
		int cellCount = player->visibleCells.size();
		bool canMerge = false;
		float mergeMass = 0.0f;
		float totalMass = 0.0f;

		for (auto cell : player->ownedCells) {
			if (cell->canMerge()) {
				mergeMass += cell->getMass();
				canMerge = true;
			}
			totalMass += cell->getMass();
		}

		if (player->ownedCells.size() > 1 && canMerge && mergeMass > totalMass / 2) {
			mouseX = player->viewArea.getX();
			mouseY = player->viewArea.getY();
			return;
		}

		// Influence calculation
		float mx = 0.0f, my = 0.0f;
		Cell* bestPrey = nullptr;
		bool splitkillObstacleNearby = false;
		float truncatedInfluence = log10f(biggestCell->getSquareSize());
		Cell* virusToSplitOn = nullptr;
		Cell* popsplitTarget = nullptr;
		list<Cell*> viruses;

		if (trypopsplit)
			for (auto [_, check] : player->visibleCells)
				if (check->getType() == CellType::VIRUS || check->getType() == CellType::MOTHER_CELL)
					viruses.push_back(check);

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
					if (trypopsplit) {
						auto count = virusInRange(viruses, check, biggestCell->getMass() * 0.75f);
						if (count == 1)
							if (splitDist < biggestCell->getSize() * 1.25f && (!popsplitTarget || check->getSize() > popsplitTarget->getSize()))
								popsplitTarget = check;
					}
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
						if (d < 60 && revpopsplit) {
							virusToSplitOn = check;
							break;
						}
						else if (d < 60 * log10f(biggestCell->getSize() / 2.0f)) {
							virusToSplitOn = check;
							break;
						}
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

			if (virusToSplitOn) break;
			if (!influence) continue;
			if (!d) d = 1.0f;
			dx /= d; dy /= d;
			mx += dx * influence / d;
			my += dy * influence / d;
		}

		if (revpopsplit && virusToSplitOn && splitCooldownTicks <= 0) {
			mouseX = virusToSplitOn->getX();
			mouseY = virusToSplitOn->getY();
			splitAttempts++;
			splitCooldownTicks = 8;
			return;
		}

		if (virusToSplitOn) {
			mouseX = virusToSplitOn->getX();
			mouseY = virusToSplitOn->getY();
			ejectAttempts++;
			ejectMacro = true;
			lockTicks = 20;
			return;
		}

		if (popsplitTarget && !popsplitTarget->isBoosting && popsplitTarget->getSize() >= 0.5f * biggestCell->getSize() && splitCooldownTicks <= 0) {
			// Logger::debug(player->leaderboardName + " popsplit -> " + popsplitTarget->owner->leaderboardName);
			mouseX = popsplitTarget->getX();
			mouseY = popsplitTarget->getY();
			splitAttempts = (randomZeroToOne * 4 + 1);
			splitCooldownTicks = splitAttempts * 2;
			lockTicks = splitAttempts * 2;
			return;
		}

		if (willingToSplit && !splitkillObstacleNearby && splitCooldownTicks <= 0 &&
			bestPrey && bestPrey->getSize() * 3.0f > biggestCell->getSize()) {
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

void PlayerBot::onDead() {
	listener->handle->ticker.timeout(60, [this] {
		if (player->cellName.length()) spawningName = player->cellName;
		else spawningName = listener->handle->randomBotName();
		if (player->cellSkin.length()) spawningSkin = player->cellSkin;
		else spawningSkin = listener->handle->randomBotSkin();
		requestSpawning = true;
	});
}