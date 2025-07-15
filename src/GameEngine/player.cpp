/**
 * @file player.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Players
 */

#ifndef LGEN_MODULE_GE_PLAYER_CPP
#define LGEN_MODULE_GE_PLAYER_CPP 1

#include "player.h"

#include "move.h"

Player::Player(const std::string& name) : name(name) {}

void Player::surrender() {
    moveQueue.clear();
    moveQueue.emplace_back(MoveType::SURRENDER, this);
}

#endif  // LGEN_MODULE_GE_PLAYER_CPP
