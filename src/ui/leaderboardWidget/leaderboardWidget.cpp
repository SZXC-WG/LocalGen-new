// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "leaderboardWidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>
#include <algorithm>

LeaderboardWidget::LeaderboardWidget(QWidget* parent) : QWidget(parent) {
    setFont(QFont("Quicksand", 10, QFont::Medium));
    setCursor(Qt::PointingHandCursor);
    layout = makeLayout();
}

void LeaderboardWidget::setRows(std::vector<LeaderboardRow> rows) {
    this->rows = std::move(rows);
    refreshSize();
    update();
}

void LeaderboardWidget::setScoreVisibility(bool showArmy, bool showLand) {
    if (this->showArmy == showArmy && this->showLand == showLand) {
        return;
    }
    this->showArmy = showArmy;
    this->showLand = showLand;
    refreshSize();
    update();
}

void LeaderboardWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen borderPen(Qt::black);
    borderPen.setWidth(2);

    const int armyColumnX = layout.player;
    const int landColumnX = armyColumnX + layout.army;

    // 1. Background
    painter.fillRect(rect(), Qt::white);

    // 2. Header
    QFont curFont = font();
    curFont.setBold(true);
    painter.setFont(curFont);
    painter.setPen(Qt::black);
    if (!collapsed) {
        painter.drawText(QRect(0, 0, layout.player, layout.header),
                         Qt::AlignCenter, QStringLiteral("Player"));
    }
    if (showArmy) {
        painter.drawText(QRect(armyColumnX, 0, layout.army, layout.header),
                         Qt::AlignCenter, QStringLiteral("Army"));
    }
    if (showLand) {
        painter.drawText(QRect(landColumnX, 0, layout.land, layout.header),
                         Qt::AlignCenter, QStringLiteral("Land"));
    }

    // 3. Body (text & icons)
    curFont.setBold(false);
    painter.setFont(curFont);

    static QSvgRenderer skullRenderer(QStringLiteral(":/images/svg/skull.svg"));

    int y = layout.header;
    for (const auto& row : rows) {
        painter.fillRect(QRect(0, y, layout.player, layout.row),
                         row.playerColor);
        painter.setPen(Qt::white);

        if (row.hasKill) {
            const int iconX = collapsed ? (layout.player - killIconSize) / 2
                                        : killIconLeftPadding;
            skullRenderer.render(
                &painter, QRect(iconX, y + (layout.row - killIconSize) / 2,
                                killIconSize, killIconSize));
        }

        if (!collapsed) {
            const int textLeft =
                row.hasKill ? killIconLeftPadding + killIconSize : 0;
            painter.drawText(
                QRect(textLeft, y, layout.player - textLeft, layout.row),
                Qt::AlignCenter, row.playerName);
        }

        painter.setPen(Qt::black);
        if (showArmy) {
            painter.drawText(QRect(armyColumnX, y, layout.army, layout.row),
                             Qt::AlignCenter, QString::number(row.army));
        }
        if (showLand) {
            painter.drawText(QRect(landColumnX, y, layout.land, layout.row),
                             Qt::AlignCenter, QString::number(row.land));
        }
        y += layout.row;
    }

    // 4. Borders
    painter.setPen(borderPen);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    borderLines.clear();
    borderLines.reserve(rows.size() + 6);
    borderLines.append(QLine(0, 0, width(), 0));
    borderLines.append(QLine(0, layout.header, width(), layout.header));
    for (int row = 1; row <= static_cast<int>(rows.size()); ++row) {
        const int lineY = layout.header + row * layout.row;
        borderLines.append(QLine(0, lineY, width(), lineY));
    }
    borderLines.append(QLine(0, 0, 0, height()));
    if (showArmy) {
        borderLines.append(QLine(armyColumnX, 0, armyColumnX, height()));
    }
    if (showLand) {
        borderLines.append(QLine(landColumnX, 0, landColumnX, height()));
    }
    borderLines.append(QLine(width(), 0, width(), height()));
    painter.drawLines(borderLines);

    // 5. Dead player overlays
    deadPlayerRects.clear();
    y = layout.header;
    for (const auto& row : rows) {
        if (!row.isAlive) {
            deadPlayerRects.append(QRect(0, y, width(), layout.row));
        }
        y += layout.row;
    }
    if (!deadPlayerRects.isEmpty()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 153));
        painter.drawRects(deadPlayerRects);
    }
}

void LeaderboardWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        collapsed = !collapsed;
        refreshSize();
        update();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void LeaderboardWidget::refreshSize() {
    layout = makeLayout();
    const QSize preferredSize(layout.width(),
                              layout.height(static_cast<int>(rows.size())));
    if (preferredSize != size()) {
        setFixedSize(preferredSize);
        emit preferredSizeChanged();
    }
}

LeaderboardWidget::Layout LeaderboardWidget::makeLayout() const {
    QFontMetrics metrics(font());
    if (!collapsed) {
        int playerWidth = metrics.horizontalAdvance(QStringLiteral("Player"));
        for (const auto& row : rows) {
            playerWidth = std::max(playerWidth,
                                   metrics.horizontalAdvance(row.playerName));
        }
        return {std::clamp(playerWidth + horizontalPadding * 2, 140, 260),
                showArmy ? scoreColumnWidth : 0,
                showLand ? scoreColumnWidth : 0, headerHeight, rowHeight};
    }

    QFont headerFont = font();
    headerFont.setBold(true);
    QFontMetrics headerMetrics(headerFont);

    bool hasKill = false;
    int armyWidth = headerMetrics.horizontalAdvance(QStringLiteral("Army"));
    int landWidth = headerMetrics.horizontalAdvance(QStringLiteral("Land"));
    for (const auto& row : rows) {
        hasKill |= row.hasKill;
        if (showArmy) {
            armyWidth =
                std::max(armyWidth,
                         metrics.horizontalAdvance(QString::number(row.army)));
        }
        if (showLand) {
            landWidth =
                std::max(landWidth,
                         metrics.horizontalAdvance(QString::number(row.land)));
        }
    }

    return {
        std::max(collapsedColorStripWidth,
                 hasKill ? killIconSize + collapsedHorizontalPadding * 2 : 0),
        showArmy ? armyWidth + collapsedHorizontalPadding * 2 : 0,
        showLand ? landWidth + collapsedHorizontalPadding * 2 : 0,
        std::max(collapsedHeaderHeight,
                 headerMetrics.height() + collapsedVerticalPadding * 2),
        std::max({collapsedRowHeight,
                  metrics.height() + collapsedVerticalPadding * 2,
                  killIconSize + collapsedVerticalPadding * 2})};
}
