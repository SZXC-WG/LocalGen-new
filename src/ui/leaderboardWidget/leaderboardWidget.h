// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QLine>
#include <QRect>
#include <QString>
#include <QVector>
#include <QWidget>
#include <vector>

#include "core/utils.hpp"

class QMouseEvent;

struct LeaderboardRow {
    QString playerName;
    army_t army = 0;
    QColor playerColor;
    int land = 0;
    bool hasKill = false;
    bool isAlive = true;
};

class LeaderboardWidget : public QWidget {
    Q_OBJECT

   public:
    explicit LeaderboardWidget(QWidget* parent = nullptr);
    void setRows(std::vector<LeaderboardRow> rows);

   signals:
    void preferredSizeChanged();

   protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

   private:
    struct Layout {
        int player = 0, army = 0, land = 0, header = 0, row = 0;
        int width() const { return player + army + land; }
        int height(int rowCount) const { return header + rowCount * row; }
    };

    void refreshSize();
    Layout makeLayout() const;

    std::vector<LeaderboardRow> rows;
    Layout layout;
    QVector<QLine> borderLines;
    QVector<QRect> deadPlayerRects;
    bool collapsed = false;

    constexpr static int headerHeight = 30;
    constexpr static int rowHeight = 28;
    constexpr static int collapsedHeaderHeight = 24;
    constexpr static int collapsedRowHeight = 22;
    constexpr static int scoreColumnWidth = 72;
    constexpr static int horizontalPadding = 12;
    constexpr static int collapsedHorizontalPadding = 6;
    constexpr static int collapsedVerticalPadding = 3;
    constexpr static int collapsedColorStripWidth = 14;
    constexpr static int killIconSize = 12;
    constexpr static int killIconLeftPadding = 6;
};
