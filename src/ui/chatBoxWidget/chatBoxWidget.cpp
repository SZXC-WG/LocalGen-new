// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "chatBoxWidget.h"

#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPalette>
#include <QScrollBar>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>

namespace {

QFont makeQuicksandFont(int pointSize, QFont::Weight weight) {
    return QFont("Quicksand", pointSize, weight);
}

void applyForegroundPalette(QWidget* widget, const QColor& color) {
    QPalette palette = widget->palette();
    palette.setColor(QPalette::WindowText, color);
    palette.setColor(QPalette::Text, color);
    palette.setColor(QPalette::ButtonText, color);
    widget->setPalette(palette);
}

void applyBackgroundPalette(QWidget* widget, const QColor& color) {
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window, color);
    palette.setColor(QPalette::Base, color);
    widget->setPalette(palette);
}

QWidget* createColorChip(const QColor& color, QWidget* parent) {
    QFrame* chip = new QFrame(parent);
    chip->setFixedSize(12, 12);
    chip->setFrameShape(QFrame::NoFrame);
    chip->setAutoFillBackground(true);

    applyBackgroundPalette(chip, color);

    return chip;
}

QLabel* createSegmentLabel(const QString& text, const QColor& color,
                           const QFont& font, QWidget* parent) {
    QLabel* label = new QLabel(text, parent);
    label->setFont(font);
    applyForegroundPalette(label, color);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    return label;
}

QWidget* createPlayerMentionWidget(const ChatMessageSegment& segment,
                                   QWidget* parent) {
    QWidget* mentionWidget = new QWidget(parent);
    mentionWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    mentionWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    QHBoxLayout* mentionLayout = new QHBoxLayout(mentionWidget);
    mentionLayout->setContentsMargins(0, 0, 0, 0);
    mentionLayout->setSpacing(8);
    mentionLayout->addWidget(
        createColorChip(segment.playerColor, mentionWidget), 0,
        Qt::AlignVCenter);
    mentionLayout->addWidget(
        createSegmentLabel(segment.text, QColor(240, 244, 248),
                           makeQuicksandFont(10, QFont::Bold), mentionWidget));

    return mentionWidget;
}

QWidget* createMessageRowWidget(const ChatMessageEntry& entry,
                                QWidget* parent) {
    QWidget* rowWidget = new QWidget(parent);
    rowWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    rowWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QHBoxLayout* layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(0, 1, 0, 1);
    layout->setSpacing(8);

    for (const ChatMessageSegment& segment : entry.segments) {
        if (segment.text.isEmpty()) continue;
        if (segment.isPlayerMention) {
            layout->addWidget(createPlayerMentionWidget(segment, rowWidget));
        } else {
            layout->addWidget(createSegmentLabel(
                segment.text, QColor(240, 244, 248),
                makeQuicksandFont(10, QFont::DemiBold), rowWidget));
        }
    }

    QLabel* turnLabel =
        createSegmentLabel(entry.turnText, QColor(184, 189, 196, 220),
                           makeQuicksandFont(9, QFont::DemiBold), rowWidget);
    turnLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    turnLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    layout->addWidget(turnLabel);
    layout->addStretch(1);

    const int boldHeight =
        QFontMetrics(makeQuicksandFont(10, QFont::Bold)).height();
    const int mediumHeight =
        QFontMetrics(makeQuicksandFont(9, QFont::DemiBold)).height();
    rowWidget->setMinimumHeight(std::max(boldHeight, mediumHeight) + 4);

    return rowWidget;
}

}  // namespace

ChatBoxWidget::ChatBoxWidget(QWidget* parent) : QFrame(parent) {
    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Plain);
    setLineWidth(1);
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);
    setForegroundRole(QPalette::WindowText);

    QPalette panelPalette = palette();
    const QColor panelBackground(0, 0, 0, 127);
    const QColor panelBorder(255, 255, 255, 36);
    panelPalette.setColor(QPalette::Window, panelBackground);
    panelPalette.setColor(QPalette::WindowText, panelBorder);
    panelPalette.setColor(QPalette::Mid, panelBorder);
    panelPalette.setColor(QPalette::Dark, panelBorder);
    setPalette(panelPalette);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget* historyContainer = new QWidget(this);
    historyContainer->setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* historyLayout = new QVBoxLayout(historyContainer);
    historyLayout->setContentsMargins(8, 6, 8, 4);
    historyLayout->setSpacing(0);

    historyList = new QListWidget(historyContainer);
    historyList->setSelectionMode(QAbstractItemView::NoSelection);
    historyList->setFocusPolicy(Qt::NoFocus);
    historyList->setWordWrap(true);
    historyList->setTextElideMode(Qt::ElideNone);
    historyList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyList->setFrameShape(QFrame::NoFrame);
    historyList->setSpacing(1);
    historyList->setAutoFillBackground(false);

    QPalette historyPalette = historyList->palette();
    historyPalette.setColor(QPalette::Base, Qt::transparent);
    historyPalette.setColor(QPalette::Text, QColor(240, 244, 248));
    historyList->setPalette(historyPalette);

    QWidget* historyViewport = historyList->viewport();
    historyViewport->setAutoFillBackground(false);
    historyViewport->setAttribute(Qt::WA_TranslucentBackground, true);

    QScrollBar* scrollBar = historyList->verticalScrollBar();
    scrollBar->setFixedWidth(10);
    historyLayout->addWidget(historyList);
    layout->addWidget(historyContainer, 1);

    inputLine = new QLineEdit(this);
    inputLine->setFixedHeight(42);
    inputLine->setFrame(false);
    inputLine->setFont(makeQuicksandFont(9, QFont::Medium));
    inputLine->setTextMargins(12, 8, 12, 8);

    QPalette inputPalette = inputLine->palette();
    inputPalette.setColor(QPalette::Base, Qt::white);
    inputPalette.setColor(QPalette::Text, QColor(0, 128, 128));
    inputPalette.setColor(QPalette::PlaceholderText, QColor(117, 117, 117));
    inputPalette.setColor(QPalette::Disabled, QPalette::Base, Qt::white);
    inputPalette.setColor(QPalette::Disabled, QPalette::Text,
                          QColor(117, 117, 117));
    inputPalette.setColor(QPalette::Disabled, QPalette::PlaceholderText,
                          QColor(117, 117, 117));
    inputLine->setPalette(inputPalette);

    inputLine->setPlaceholderText("Press [Enter] to chat.");
    connect(inputLine, &QLineEdit::returnPressed, this,
            &ChatBoxWidget::emitPendingMessage);
    layout->addWidget(inputLine);
}

void ChatBoxWidget::appendMessage(const ChatMessageEntry& message) {
    if (message.segments.empty()) {
        return;
    }

    auto* item = new QListWidgetItem();
    QWidget* rowWidget = createMessageRowWidget(message, historyList);
    rowWidget->ensurePolished();
    const QSize rowSize = rowWidget->sizeHint();
    item->setSizeHint(QSize(rowSize.width(), rowSize.height() + 2));
    historyList->addItem(item);
    historyList->setItemWidget(item, rowWidget);

    while (historyList->count() > maxHistoryEntries) {
        QListWidgetItem* oldItem = historyList->item(0);
        QWidget* oldWidget = historyList->itemWidget(oldItem);
        historyList->removeItemWidget(oldItem);
        delete oldWidget;
        oldItem = historyList->takeItem(0);
        delete oldItem;
    }
    historyList->scrollToBottom();
    QScrollBar* scrollBar = historyList->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ChatBoxWidget::focusInput() {
    if (!inputLine->isEnabled()) return;
    inputLine->setFocus(Qt::ShortcutFocusReason);
}

void ChatBoxWidget::setInputEnabled(bool enabled, const QString& placeholder) {
    inputLine->setEnabled(enabled);
    inputLine->setPlaceholderText(placeholder);
    if (!enabled) inputLine->clear();
}

void ChatBoxWidget::emitPendingMessage() {
    const QString message = inputLine->text();
    if (!message.isEmpty()) {
        inputLine->clear();
        emit messageSubmitted(message);
    }
}
