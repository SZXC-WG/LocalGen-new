// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QPaintEvent>
#include <QString>
#include <QWidget>
#include <functional>
#include <vector>

#include "core/utils.hpp"

struct LeaderboardRow {
    index_t playerId = -1;
    QString playerName;
    army_t army = 0;
    int land = 0;
    int killCount = 0;
    QColor playerColor;
    bool isAlive = true;
};

class LeaderboardWidget : public QWidget {
   public:
    struct Column {
        QString title;
        int width = 80;
        int minWidth = 0;
        int maxWidth = 0;
        std::function<QString(const LeaderboardRow&)> textProvider;
        std::function<QColor(const LeaderboardRow&)> backgroundProvider;
        std::function<QColor(const LeaderboardRow&)> foregroundProvider;
        bool showKillIcon = false;
    };

    explicit LeaderboardWidget(QWidget* parent = nullptr);

    void setColumns(std::vector<Column> columns);
    void setRows(std::vector<LeaderboardRow> rows);

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    std::vector<int> computeColumnWidths() const;
    void updateFixedSize();

    std::vector<Column> columns;
    std::vector<LeaderboardRow> rows;
    std::vector<int> columnWidths;
    constexpr static int headerHeight = 30;
    constexpr static int rowHeight = 28;
    constexpr static int horizontalPadding = 12;
    constexpr static int killIconLeftPadding = 6;
    constexpr static int killIconSize = 12;
};
