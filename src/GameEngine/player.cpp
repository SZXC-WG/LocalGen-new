/**
 * @file player.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Players
 *
 * Players are basic participants of games.
 */

#ifndef LGEN_MODULE_GE_PLAYER_CPP
#define LGEN_MODULE_GE_PLAYER_CPP 1

#include "player.h"

#include "move.h"

Player::Player(const std::string& name) : name(name) {}
Player::~Player() {}

#endif  // LGEN_MODULE_GE_PLAYER_CPP
