// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractButton>
#include <QDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QString>
#include <QVBoxLayout>

#include "generalsButton.hpp"

class SurrenderConfirmDialog : public QDialog {
   public:
    explicit SurrenderConfirmDialog(QWidget* parent = nullptr)
        : QDialog(parent) {
        setObjectName("surrenderConfirmDialog");
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);

        auto* dialogLayout = new QVBoxLayout(this);
        dialogLayout->setContentsMargins(0, 0, 0, 0);

        auto* panel = new QFrame(this);
        panel->setObjectName("surrenderConfirmPanel");
        panel->setAttribute(Qt::WA_StyledBackground, true);
        panel->setFixedWidth(410);
        panel->setStyleSheet(
            "QFrame#surrenderConfirmPanel { background-color: rgba(18, 21, "
            "26, 232); border: 1px solid rgba(255, 255, 255, 46); "
            "border-radius: 8px; }"
            "QLabel { background: transparent; border: none; color: rgb(240, "
            "244, 248); }"
            "QLabel#surrenderConfirmBody { color: rgba(240, 244, 248, 204); }");

        auto* panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(18, 16, 18, 18);
        panelLayout->setSpacing(12);

        auto* accentBar = new QFrame(panel);
        accentBar->setFixedHeight(4);
        accentBar->setAttribute(Qt::WA_StyledBackground, true);
        accentBar->setStyleSheet(
            "QFrame { background-color: rgb(0, 128, 128); border-radius: 2px; "
            "}");
        panelLayout->addWidget(accentBar);

        auto* titleLabel = new QLabel("Surrender?", panel);
        titleLabel->setFont(QFont("Quicksand", 14, QFont::Bold));
        panelLayout->addWidget(titleLabel);

        auto* bodyLabel = new QLabel(
            "Are you sure you want to surrender? You will lose control of your "
            "armies and become a spectator.",
            panel);
        bodyLabel->setObjectName("surrenderConfirmBody");
        bodyLabel->setFont(QFont("Quicksand", 10, QFont::DemiBold));
        bodyLabel->setWordWrap(true);
        panelLayout->addWidget(bodyLabel);

        auto* buttons = new QHBoxLayout();
        buttons->setContentsMargins(0, 4, 0, 0);
        buttons->setSpacing(12);
        buttons->addStretch(1);

        auto* surrenderButton = makeButton("Surrender", panel);
        auto* keepPlayingButton = makeButton("Keep Playing", panel);
        connect(surrenderButton, &QAbstractButton::clicked, this,
                &QDialog::accept);
        connect(keepPlayingButton, &QAbstractButton::clicked, this,
                &QDialog::reject);

        buttons->addWidget(surrenderButton);
        buttons->addWidget(keepPlayingButton);
        panelLayout->addLayout(buttons);
        dialogLayout->addWidget(panel);

        centerOnParent(parent);
        keepPlayingButton->setFocus(Qt::OtherFocusReason);
    }

   private:
    static GeneralsButton* makeButton(const QString& text, QWidget* parent) {
        auto* button = new GeneralsButton(parent);
        button->setText(text);
        button->setFont(QFont("Quicksand", 10, QFont::Bold));
        button->setFixedSize(138, 40);
        return button;
    }

    void centerOnParent(QWidget* parent) {
        adjustSize();
        if (parent == nullptr) return;

        move(parent->mapToGlobal(parent->rect().center()) -
             QPoint(width() / 2, height() / 2));
    }
};
