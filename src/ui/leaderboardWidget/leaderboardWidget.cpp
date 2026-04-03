// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "leaderboardWidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <algorithm>

LeaderboardWidget::LeaderboardWidget(QWidget* parent) : QWidget(parent) {
    setFont(QFont("Quicksand", 10, QFont::Medium));
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void LeaderboardWidget::setColumns(std::vector<Column> columns) {
    this->columns = std::move(columns);
    updateFixedSize();
    update();
}

void LeaderboardWidget::setRows(std::vector<LeaderboardRow> rows) {
    this->rows = std::move(rows);
    updateFixedSize();
    update();
}

void LeaderboardWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (columns.empty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen borderPen(Qt::black);
    borderPen.setWidth(2);
    painter.setPen(borderPen);

    std::vector<int> columnWidths = computeColumnWidths();

    QFont headerFont = font();
    headerFont.setBold(true);

    int x = 0;
    for (size_t i = 0; i < columns.size(); ++i) {
        const auto& column = columns[i];
        QRect cellRect(x, 0, columnWidths[i], headerHeight);
        painter.fillRect(cellRect, Qt::white);
        painter.drawRect(cellRect);
        painter.setFont(headerFont);
        painter.setPen(Qt::black);
        painter.drawText(cellRect, Qt::AlignCenter, column.title);
        painter.setPen(borderPen);
        x += columnWidths[i];
    }

    QFont bodyFont = font();
    bodyFont.setBold(false);
    painter.setFont(bodyFont);
    QFontMetrics metrics(bodyFont);

    int y = headerHeight;
    for (const auto& row : rows) {
        int rowX = 0;
        for (size_t i = 0; i < columns.size(); ++i) {
            const auto& column = columns[i];
            QRect cellRect(rowX, y, columnWidths[i], rowHeight);
            QColor background = Qt::white;
            QColor foreground = Qt::black;
            QString text;

            if (column.backgroundProvider) {
                background = column.backgroundProvider(row);
            }
            if (column.foregroundProvider) {
                foreground = column.foregroundProvider(row);
            }
            if (column.textProvider) {
                text = column.textProvider(row);
            }

            painter.fillRect(cellRect, background);
            painter.setPen(borderPen);
            painter.drawRect(cellRect);
            painter.setPen(foreground);

            QString displayText = metrics.elidedText(
                text, Qt::ElideRight, cellRect.width() - horizontalPadding);
            painter.drawText(cellRect, Qt::AlignCenter, displayText);

            painter.setPen(borderPen);
            rowX += columnWidths[i];
        }

        if (!row.isAlive) {
            painter.fillRect(QRect(0, y, width(), rowHeight),
                             QColor(0, 0, 0, 153));
        }

        y += rowHeight;
    }

    QPen outerBorderPen(Qt::black);
    outerBorderPen.setWidth(3);
    painter.setPen(outerBorderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
}

std::vector<int> LeaderboardWidget::computeColumnWidths() const {
    std::vector<int> widths;
    widths.reserve(columns.size());

    QFontMetrics metrics(font());
    for (const auto& column : columns) {
        int width = column.width;
        if (width <= 0) {
            int maxTextWidth = metrics.horizontalAdvance(column.title);
            if (column.textProvider) {
                for (const auto& row : rows) {
                    maxTextWidth = std::max(
                        maxTextWidth,
                        metrics.horizontalAdvance(column.textProvider(row)));
                }
            }
            width = maxTextWidth + horizontalPadding * 2;
        }

        if (column.minWidth > 0) {
            width = std::max(width, column.minWidth);
        }
        if (column.maxWidth > 0) {
            width = std::min(width, column.maxWidth);
        }
        widths.push_back(width);
    }

    return widths;
}

void LeaderboardWidget::updateFixedSize() {
    int totalWidth = 0;
    for (int columnWidth : computeColumnWidths()) {
        totalWidth += columnWidth;
    }
    int height = headerHeight + static_cast<int>(rows.size()) * rowHeight;
    setFixedSize(totalWidth, height);
}
