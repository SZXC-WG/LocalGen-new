/**
 * @file tools.h
 *
 * LocalGen Library: Tools
 *
 * Useful overall tools that can be used wherever the code is.
 */

#ifndef LGEN_LIB_TOOLS
#define LGEN_LIB_TOOLS 1

#include <random>

/// default random-integer generators

std::mt19937 random{std::random_device()()};
std::mt19937_64 random64{std::random_device()()};

#endif
