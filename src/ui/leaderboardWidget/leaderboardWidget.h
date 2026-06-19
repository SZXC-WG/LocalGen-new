// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QString>
#include <QWidget>
#include <vector>

#include "core/utils.hpp"

struct LeaderboardRow {
    QString playerName;
    army_t army = 0;
    QColor playerColor;
    int land = 0;
    bool hasKill = false;
    bool isAlive = true;
};

class LeaderboardWidget : public QWidget {
   public:
    explicit LeaderboardWidget(QWidget* parent = nullptr);
    void setRows(std::vector<LeaderboardRow> rows);

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    int playerColumnWidth() const;

    std::vector<LeaderboardRow> rows;
    constexpr static int headerHeight = 30;
    constexpr static int rowHeight = 28;
    constexpr static int scoreColumnWidth = 72;
    constexpr static int horizontalPadding = 12;
    constexpr static int killIconSize = 12;
    constexpr static int killIconLeftPadding = 6;
    constexpr static int killIconTopPadding = (rowHeight - killIconSize) / 2;
};
