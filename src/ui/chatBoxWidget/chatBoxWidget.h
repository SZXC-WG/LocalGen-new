// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QFrame>
#include <QString>
#include <vector>

class QLineEdit;
class QListWidget;

struct ChatMessageSegment {
    QString text;
    bool isPlayerMention = false;
    QColor playerColor;
};

struct ChatMessageEntry {
    std::vector<ChatMessageSegment> segments;
    QString turnText;
};

class ChatBoxWidget : public QFrame {
    Q_OBJECT

   public:
    explicit ChatBoxWidget(QWidget* parent = nullptr);

    void appendMessage(const ChatMessageEntry& message);
    void focusInput();
    void setInputEnabled(bool enabled, const QString& placeholder = QString());

   signals:
    void messageSubmitted(const QString& message);

   private:
    void emitPendingMessage();

    QListWidget* historyList = nullptr;
    QLineEdit* inputLine = nullptr;

    static constexpr int maxHistoryEntries = 100;
};
