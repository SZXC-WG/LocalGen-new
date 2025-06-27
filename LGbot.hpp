/* This is LGbot.hpp file of LocalGen.                                  */
/* Copyright (c) 2023 LocalGen-dev; All rights reserved.                 */
/* Developers: http://github.com/LocalGen-dev                            */
/* Project: http://github.com/LocalGen-dev/LocalGen-new                  */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/LocalGen-dev/LocalGen-new/blob/main/LICENSE.md      */

#ifndef LGBOT_HPP_
#define LGBOT_HPP_

// Bots
// #include "botlib/ktqBot.hpp"
#include "botlib/gcBot.hpp"
#include "botlib/smartRandomBot.hpp"
#include "botlib/szlyBot.hpp"
#include "botlib/xiaruizeBot.hpp"
#include "botlib/xrzBot.hpp"
#include "botlib/zlyBot.hpp"
#include "botlib/zlyBot_v2.hpp"
// #include "botlib/zlyBot_v3.hpp"

string botName[] = {
    "",        "smartRandomBot", "xrzBot", "xiaruizeBot", "zlyBot",
    "szlyBot", "zlyBot v2",      "gcBot",  "zlyBot v3",   "gjrBot",
};

#endif  // LGBOT_HPP_
