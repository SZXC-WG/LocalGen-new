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
#include <QScrollBar>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {

QWidget* createColorChip(const QColor& color, QWidget* parent) {
    QFrame* chip = new QFrame(parent);
    chip->setFixedSize(12, 12);
    chip->setStyleSheet(QString("QFrame { background-color: %1; border-radius: "
                                "0px; border: none; }")
                            .arg(color.name()));
    return chip;
}

QLabel* createSegmentLabel(const QString& text, const QString& style,
                           QWidget* parent) {
    QLabel* label = new QLabel(text, parent);
    label->setStyleSheet(style);
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
        createSegmentLabel(segment.text,
                           "QLabel { color: rgb(240, 244, 248); "
                           "font: 700 10pt 'Quicksand'; }",
                           mentionWidget));

    return mentionWidget;
}

QWidget* createMessageRowWidget(const ChatMessageEntry& entry,
                                QWidget* parent) {
    QWidget* rowWidget = new QWidget(parent);
    rowWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    rowWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QHBoxLayout* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 1, 0, 1);
    rowLayout->setSpacing(0);

    QWidget* contentWidget = new QWidget(rowWidget);
    QHBoxLayout* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    for (const ChatMessageSegment& segment : entry.segments) {
        if (segment.text.isEmpty()) {
            continue;
        }

        if (segment.isPlayerMention) {
            contentLayout->addWidget(
                createPlayerMentionWidget(segment, contentWidget));
            continue;
        }

        contentLayout->addWidget(
            createSegmentLabel(segment.text,
                               "QLabel { color: rgb(240, 244, 248); "
                               "font: 600 10pt 'Quicksand'; }",
                               contentWidget));
    }

    QLabel* turnLabel =
        createSegmentLabel(entry.turnText,
                           "QLabel { color: rgba(184, 189, 196, 220); "
                           "font: 600 9pt 'Quicksand'; }",
                           contentWidget);
    turnLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    turnLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    contentLayout->addWidget(turnLabel);
    contentLayout->addStretch(1);
    rowLayout->addWidget(contentWidget, 1);

    const int boldHeight =
        QFontMetrics(QFont("Quicksand", 10, QFont::Bold)).height();
    const int mediumHeight =
        QFontMetrics(QFont("Quicksand", 9, QFont::DemiBold)).height();
    rowWidget->setMinimumHeight(std::max(boldHeight, mediumHeight) + 4);

    return rowWidget;
}

}  // namespace

ChatBoxWidget::ChatBoxWidget(QWidget* parent) : QFrame(parent) {
    setFocusPolicy(Qt::NoFocus);
    setObjectName("chatBoxWidget");
    setFrameShape(QFrame::NoFrame);
    setStyleSheet(
        "QFrame#chatBoxWidget {"
        "background-color: rgba(0, 0, 0, 127);"
        "border: 1px solid rgba(255, 255, 255, 36);"
        "border-radius: 3px;"
        "}"
        "QLabel {"
        "color: rgb(240, 244, 248);"
        "font: 700 10pt 'Quicksand';"
        "}"
        "QListWidget {"
        "background-color: transparent;"
        "color: rgb(240, 244, 248);"
        "border: none;"
        "padding: 6px 8px 4px 8px;"
        "outline: none;"
        "}"
        "QListWidget::item {"
        "padding: 0px;"
        "margin: 0px;"
        "border: none;"
        "}"
        "QListWidget::item:selected {"
        "background: transparent;"
        "}"
        "QScrollBar:vertical {"
        "background: transparent;"
        "width: 10px;"
        "margin: 6px 2px 6px 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "background: rgba(255, 255, 255, 58);"
        "border-radius: 4px;"
        "min-height: 24px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "height: 0px;"
        "background: transparent;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "background: transparent;"
        "}"
        "QLineEdit {"
        "background-color: rgb(246, 246, 246);"
        "color: rgb(36, 36, 36);"
        "border: none;"
        "border-top: 1px solid rgba(0, 0, 0, 34);"
        "border-radius: 0px;"
        "padding: 8px 12px;"
        "font: 500 9pt 'Quicksand';"
        "}"
        "QLineEdit::placeholder { color: rgb(126, 130, 137); }"
        "QLineEdit:disabled {"
        "color: rgb(126, 130, 137);"
        "background-color: rgb(238, 238, 238);"
        "}");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    historyList = new QListWidget(this);
    historyList->setSelectionMode(QAbstractItemView::NoSelection);
    historyList->setFocusPolicy(Qt::NoFocus);
    historyList->setWordWrap(true);
    historyList->setTextElideMode(Qt::ElideNone);
    historyList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyList->setFrameShape(QFrame::NoFrame);
    historyList->setSpacing(1);
    layout->addWidget(historyList, 1);

    inputLine = new QLineEdit(this);
    inputLine->setFixedHeight(42);
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
        if (oldItem != nullptr) {
            QWidget* oldWidget = historyList->itemWidget(oldItem);
            if (oldWidget != nullptr) {
                historyList->removeItemWidget(oldItem);
                delete oldWidget;
            }
            oldItem = historyList->takeItem(0);
            delete oldItem;
        }
    }
    historyList->scrollToBottom();
    if (QScrollBar* scrollBar = historyList->verticalScrollBar();
        scrollBar != nullptr) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void ChatBoxWidget::clearInput() { inputLine->clear(); }

void ChatBoxWidget::focusInput() {
    if (inputLine == nullptr || !inputLine->isEnabled()) {
        return;
    }
    inputLine->setFocus(Qt::ShortcutFocusReason);
}

void ChatBoxWidget::setInputEnabled(bool enabled, const QString& placeholder) {
    inputLine->setEnabled(enabled);
    inputLine->setPlaceholderText(placeholder);
    if (!enabled) {
        inputLine->clear();
    }
}

void ChatBoxWidget::emitPendingMessage() {
    const QString message = inputLine->text();
    if (message.isEmpty()) {
        return;
    }
    emit messageSubmitted(message);
}
