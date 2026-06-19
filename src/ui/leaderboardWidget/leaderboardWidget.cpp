// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "leaderboardWidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QLine>
#include <QPainter>
#include <QSvgRenderer>
#include <QVector>
#include <algorithm>

LeaderboardWidget::LeaderboardWidget(QWidget* parent) : QWidget(parent) {
    setFont(QFont("Quicksand", 10, QFont::Medium));
}

void LeaderboardWidget::setRows(std::vector<LeaderboardRow> rows) {
    this->rows = std::move(rows);
    setFixedSize(
        playerColumnWidth() + scoreColumnWidth * 2,
        headerHeight + static_cast<int>(this->rows.size()) * rowHeight);
    update();
}

void LeaderboardWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen borderPen(Qt::black);
    borderPen.setWidth(2);

    QFont headerFont = font();
    headerFont.setBold(true);

    const int playerWidth = playerColumnWidth();
    const int armyColumnX = playerWidth;
    const int landColumnX = playerWidth + scoreColumnWidth;

    // 1. Background
    painter.fillRect(rect(), Qt::white);

    // 2. Player color blocks
    int y = headerHeight;
    for (const auto& row : rows) {
        painter.fillRect(QRect(0, y, playerWidth, rowHeight), row.playerColor);
        y += rowHeight;
    }

    // 3. Text & icons
    painter.setFont(headerFont);
    painter.setPen(Qt::black);
    painter.drawText(QRect(0, 0, playerWidth, headerHeight), Qt::AlignCenter,
                     QStringLiteral("Player"));
    painter.drawText(QRect(armyColumnX, 0, scoreColumnWidth, headerHeight),
                     Qt::AlignCenter, QStringLiteral("Army"));
    painter.drawText(QRect(landColumnX, 0, scoreColumnWidth, headerHeight),
                     Qt::AlignCenter, QStringLiteral("Land"));

    QFont bodyFont = font();
    bodyFont.setBold(false);
    painter.setFont(bodyFont);

    static QSvgRenderer skullRenderer(QString(":/images/svg/skull.svg"));

    y = headerHeight;
    for (const auto& row : rows) {
        const QRect playerRect(0, y, playerWidth, rowHeight);

        painter.setPen(Qt::white);

        QRect playerTextRect = playerRect;
        if (row.hasKill) {
            const QRect iconRect(playerRect.left() + killIconLeftPadding,
                                 playerRect.top() + killIconTopPadding,
                                 killIconSize, killIconSize);
            skullRenderer.render(&painter, iconRect);

            playerTextRect.setLeft(playerRect.left() + killIconLeftPadding +
                                   killIconSize);
        }

        painter.drawText(playerTextRect, Qt::AlignCenter, row.playerName);

        painter.setPen(Qt::black);
        painter.drawText(QRect(armyColumnX, y, scoreColumnWidth, rowHeight),
                         Qt::AlignCenter, QString::number(row.army));
        painter.drawText(QRect(landColumnX, y, scoreColumnWidth, rowHeight),
                         Qt::AlignCenter, QString::number(row.land));

        y += rowHeight;
    }

    // 4. Table borders
    QVector<QLine> borderLines;
    borderLines.reserve(rows.size() + 6);
    borderLines.append(QLine(0, 0, width(), 0));
    borderLines.append(QLine(0, headerHeight, width(), headerHeight));
    for (int row = 1; row <= static_cast<int>(rows.size()); ++row) {
        const int lineY = headerHeight + row * rowHeight;
        borderLines.append(QLine(0, lineY, width(), lineY));
    }
    borderLines.append(QLine(0, 0, 0, height()));
    borderLines.append(QLine(armyColumnX, 0, armyColumnX, height()));
    borderLines.append(QLine(landColumnX, 0, landColumnX, height()));
    borderLines.append(QLine(width(), 0, width(), height()));

    painter.setPen(borderPen);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    painter.drawLines(borderLines);

    // 5. Dead player overlays
    QVector<QRect> deadPlayerRects;
    y = headerHeight;
    for (const auto& row : rows) {
        if (!row.isAlive) {
            deadPlayerRects.append(QRect(0, y, width(), rowHeight));
        }
        y += rowHeight;
    }
    if (!deadPlayerRects.isEmpty()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 153));
        painter.drawRects(deadPlayerRects);
    }
}

int LeaderboardWidget::playerColumnWidth() const {
    QFontMetrics metrics(font());
    int maxTextWidth = metrics.horizontalAdvance(QStringLiteral("Player"));
    for (const auto& row : rows) {
        maxTextWidth =
            std::max(maxTextWidth, metrics.horizontalAdvance(row.playerName));
    }

    return std::clamp(maxTextWidth + horizontalPadding * 2, 140, 260);
}
