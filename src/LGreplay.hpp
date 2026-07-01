/* This is LGreplay.hpp file of LocalGen.                                */
/* Copyright (c) 2024 SZXC Work Group; All rights reserved.              */
/* Developers: http://github.com/SZXC-WG                                 */
/* Project: http://github.com/SZXC-WG/LocalGen-new                       */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/SZXC-WG/LocalGen-new/blob/main/LICENSE.md           */

#ifndef LGREPLAY_HPP_
#define LGREPLAY_HPP_

#include "LGdef.hpp"

namespace LGreplay {
char ntoc(int x) {
    switch (x) {
        case 0 ... 25:  return x + 65;
        case 26 ... 51: return x + 71;
        case 52 ... 61: return x - 4;
        case 62:        return '+';
        case 63:        return '/';
        default:        return '_';
    }
}
int cton(char x) {
    switch (x) {
        case 65 ... 90:  return x - 65;
        case 97 ... 122: return x - 71;
        case 48 ... 57:  return x + 4;
        case '+':        return 62;
        case '/':        return 63;
        default:         return 0;
    }
}
string ntos(int x, int len) {
    string res = "";
    while (x) {
        res += ntoc(x & 63);
        x >>= 6;
    }
    if (len != -1)
        while (res.size() < len) res += 'A';
    len = res.size();
    for (int i = 0; i < len / 2; ++i) std::swap(res[i], res[len - i - 1]);
    return res;
}
int ston(char* s, int len) {
    if (len == -1) len = strlen(s);
    int res = 0;
    for (int i = 0; i < len; ++i) res = res << 6 | cton(s[i]);
    return res;
}
string zipBlock(Block B) {
    string res = "";
    res += ntoc(B.lit << 3 | B.type);
    res += ntoc(B.player << 1 | (B.army < 0));
    res += ntos(std::abs(B.army), 4);
    return res;
}

Movement::Movement() {}
Movement::Movement(moveS m) { move = m; }
string Movement::zip() {
    string res = "";
    res += ntoc(move.id << 1 | move.takeArmy);
    res += ntos(move.from.x, 2);
    res += ntos(move.from.y, 2);
    res += ntos(move.to.x, 2);
    res += ntos(move.to.y, 2);
    return res;
}
Movement readMove(char* buf) {
    Movement mov;
    mov.move = moveS{(cton(buf[0]) >> 1), bool(cton(buf[0]) & 1),
                     coordS{ston(buf + 1, 2), ston(buf + 3, 2)},
                     coordS{ston(buf + 5, 2), ston(buf + 7, 2)}};
    return mov;
}

WReplay::WReplay(string Fname) { Filename = Fname; }
void WReplay::initReplay(string Fname) {
    Filename =
        Fname + "_" +
        std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()) +
        ".lgr";
    file = fopen(Filename.c_str(), "w");
    string info = "";
    info += ntoc(LGgame::playerCnt);
    info += ntos(mapH, 2);
    info += ntos(mapW, 2);
    fprintf(file, info.c_str());
    for (int i = 1; i <= mapH; ++i)
        for (int j = 1; j <= mapW; ++j)
            fprintf(file, zipBlock(gameMap[i][j]).c_str());
}
void WReplay::newTurn() { fprintf(file, "_"); }
void WReplay::newMove(Movement mov) { fprintf(file, mov.zip().c_str()); }

string ts(int x) {
    string s = " ";
    while (x) {
        s = (char)(x % 10 + 48) + s;
        x /= 10;
    }
    return s;
}
Block readBlock(FILE* fp) {
    char* readBuf = new char[6];
    fread(readBuf, 1, 6, fp);
    Block B;
    B.lit = cton(readBuf[0]) >> 3;
    B.type = cton(readBuf[0]) & 7;
    int t = cton(readBuf[1]);
    B.player = t >> 1;
    int nega = 1;
    if (t & 1) nega = -1;
    B.army = ston(readBuf + 2, 4) * nega;
    return B;
}
int QwQ(Movement mov) {
    LGgame::inlineMove.push_back(mov.move);
    return 0;
}
void updMap(int turn) {
    for (int i = 1; i <= mapH; ++i) {
        for (int j = 1; j <= mapW; ++j) {
            if (gameMap[i][j].player == 0) continue;
            switch (gameMap[i][j].type) {
                case 0: {
                    if (turn % 25 == 0) ++gameMap[i][j].army;
                    break;
                }
                case 1: {
                    if (gameMap[i][j].army > 0)
                        if (!(--gameMap[i][j].army)) gameMap[i][j].player = 0;
                    break;
                }
                case 2: break;
                case 3: {
                    ++gameMap[i][j].army;
                    break;
                }
                case 4: {
                    ++gameMap[i][j].army;
                    break;
                }
            }
        }
    }
}
void replayMap::download() {
    for (int i = 1; i <= mapH; ++i)
        for (int j = 1; j <= mapW; ++j) rMap[i][j] = gameMap[i][j];
    for (int i = 1; i <= LGgame::playerCnt; ++i) alive[i] = LGgame::isAlive[i];
}
void replayMap::upload() {
    for (int i = 1; i <= mapH; ++i)
        for (int j = 1; j <= mapW; ++j) gameMap[i][j] = rMap[i][j];
    for (int i = 1; i <= LGgame::playerCnt; ++i) LGgame::isAlive[i] = alive[i];
}
RReplay::RReplay(string Fname) { Filename = Fname; }
void RReplay::resetGame() {
    for (int i = 1; i <= mapH; ++i)
        for (int j = 1; j <= mapW; ++j) gameMap[i][j] = startMap[i][j];
    for (int i = 1; i <= LGgame::playerCnt; ++i) LGgame::isAlive[i] = 1;
}
bool RReplay::_nextTurn() {
    updMap(curTurn);
    ++curTurn;
    while (1) {
        if (fread(readBuf, 1, 1, file) != 1) return 1;
        ++seekPos;
        if (readBuf[0] == '_') break;
        fread(readBuf + 1, 1, 8, file);
        seekPos += 8;
        QwQ(readMove(readBuf));
    }
    LGgame::flushMove();
    return 0;
}
int RReplay::nextTurn() {
    if (curTurn == totTurn) return 1;
    updMap(curTurn);
    ++curTurn;
    fseek(file, 1, SEEK_CUR);
    ++seekPos;
    while (seekPos < turnPos[curTurn]) {
        fread(readBuf, 1, 9, file);
        seekPos += 9;
        QwQ(readMove(readBuf));
    }
    LGgame::flushMove();
    return 0;
}
void RReplay::gotoTurn(int turnid) {
    if (turnid < 0 || turnid > totTurn) return;
    // if(turnid==0) {
    // 	resetGame();
    // 	return;
    // }
    curTurn = 0;
    seekPos = turnPos[curTurn];
    fseek(file, seekPos, SEEK_SET);
    resetGame();
    while (curTurn < turnid) nextTurn();
}
int RReplay::preTurn() {
    if (curTurn == 0) return 1;
    gotoTurn(curTurn - 1);
    return 0;
}
void RReplay::initReplay(string Fname) {
    LGgame::inReplay = true;
    Filename = Fname + ".lgr";
    file = fopen(Filename.c_str(), "r");
    if (!file) {
        MessageBoxW(nullptr, L"No such replay file!", L"Local Generals.io",
                    MB_OK);
        closegraph();
        exit(0);
    }
    seekPos = 0;
    fseek(file, 0, SEEK_SET);
    fread(readBuf, 1, 5, file);
    seekPos += 5;
    LGgame::playerCnt = ston(readBuf, 1);
    mapH = ston(readBuf + 1, 2);
    mapW = ston(readBuf + 3, 2);
    for (int i = 1; i <= mapH; ++i)
        for (int j = 1; j <= mapW; ++j)
            gameMap[i][j] = readBlock(file), seekPos += 6;
    for (int i = 1; i <= mapH; ++i)
        for (int j = 1; j <= mapW; ++j) startMap[i][j] = gameMap[i][j];
    for (int i = 1; i <= mapH; ++i)
        for (int j = 1; j <= mapW; ++j)
            if (gameMap[i][j].type == 3)
                LGgame::genCoo[gameMap[i][j].player] = coordS(i, j);
    fseek(file, 1, SEEK_CUR);
    ++seekPos;
    totTurn = 0;
    curTurn = 0;
    midStates.clear();
    replayMap rmap;
    rmap.download();
    midStates.push_back(rmap);
    while (1) {
        turnPos[totTurn++] = seekPos - 1;
        if (_nextTurn()) break;
        if (totTurn % 100 == 0) {
            replayMap nrmap;
            nrmap.download();
            midStates.push_back(nrmap);
        }
    }
    replaySize = seekPos;
    seekPos = turnPos[0];
    fseek(file, turnPos[0], SEEK_SET);
    curTurn = 0;
    resetGame();
}
}  // namespace LGreplay

#endif  // LGREPLAY_HPP_
