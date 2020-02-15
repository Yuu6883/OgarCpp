#include "Player.h"
#include "../worlds/World.h"
#include "../ServerHandle.h"

Player::Player (ServerHandle* handle, unsigned int id, Router* router) :
	handle(handle), id(id), router(router) {
	double scale = handle->getSettingDouble("playerViewScaleMult");
	viewArea.w /= scale;
	viewArea.h /= scale;
};

Player::~Player() {
	if (hasWorld) world->removePlayer(this);
	exists = false;
};

void Player::updateState(PlayerState state) {

};