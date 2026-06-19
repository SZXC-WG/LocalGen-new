// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "leaderboardWidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QSvgRenderer>
#include <algorithm>

LeaderboardWidget::LeaderboardWidget(QWidget* parent) : QWidget(parent) {
    setFont(QFont("Quicksand", 10, QFont::Medium));
    setAttribute(Qt::WA_TranslucentBackground, true);
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
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen borderPen(Qt::black);
    borderPen.setWidth(2);

    QFont headerFont = font();
    headerFont.setBold(true);

    const int playerWidth = playerColumnWidth();
    auto drawCell = [&](const QRect& cellRect, const QColor& background,
                        const QColor& foreground, const QString& text) {
        painter.fillRect(cellRect, background);
        painter.setPen(borderPen);
        painter.drawRect(cellRect);
        painter.setPen(foreground);
        painter.drawText(cellRect, Qt::AlignCenter, text);
        painter.setPen(borderPen);
    };

    painter.setFont(headerFont);
    painter.setPen(borderPen);
    drawCell(QRect(0, 0, playerWidth, headerHeight), Qt::white, Qt::black,
             QStringLiteral("Player"));
    drawCell(QRect(playerWidth, 0, scoreColumnWidth, headerHeight), Qt::white,
             Qt::black, QStringLiteral("Army"));
    drawCell(QRect(playerWidth + scoreColumnWidth, 0, scoreColumnWidth,
                   headerHeight),
             Qt::white, Qt::black, QStringLiteral("Land"));

    QFont bodyFont = font();
    bodyFont.setBold(false);
    painter.setFont(bodyFont);

    int y = headerHeight;
    for (const auto& row : rows) {
        const QRect playerRect(0, y, playerWidth, rowHeight);

        painter.fillRect(playerRect, row.playerColor);
        painter.setPen(borderPen);
        painter.drawRect(playerRect);
        painter.setPen(Qt::white);

        QRect playerTextRect = playerRect;
        if (row.hasKill) {
            static QSvgRenderer skullRenderer(
                QString(":/images/svg/skull.svg"));
            const QRect iconRect(playerRect.left() + killIconLeftPadding,
                                 playerRect.top() + killIconTopPadding,
                                 killIconSize, killIconSize);
            skullRenderer.render(&painter, iconRect);

            playerTextRect.setLeft(playerRect.left() + killIconLeftPadding +
                                   killIconSize);
        }

        painter.drawText(playerTextRect, Qt::AlignCenter, row.playerName);

        drawCell(QRect(playerWidth, y, scoreColumnWidth, rowHeight), Qt::white,
                 Qt::black, QString::number(row.army));
        drawCell(QRect(playerWidth + scoreColumnWidth, y, scoreColumnWidth,
                       rowHeight),
                 Qt::white, Qt::black, QString::number(row.land));

        if (!row.isAlive) {
            painter.fillRect(QRect(0, y, width(), rowHeight),
                             QColor(0, 0, 0, 153));
        }

        y += rowHeight;
    }

    borderPen.setWidth(3);
    painter.setPen(borderPen);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
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
