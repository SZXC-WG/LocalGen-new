// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDialog>
#include <QStringList>

#include "core/game-config.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class LocalGameDialog;
}
QT_END_NAMESPACE

struct LocalGameConfig {
    int gameSpeed;
    bool enableSounds, showAnalysis;
    QString mapFilePath;
    int mapWidth, mapHeight;
    QStringList players;
    GameConfig gameConfig = defaultConf;
};

class LocalGameDialog : public QDialog {
    Q_OBJECT

   public:
    explicit LocalGameDialog(QWidget* parent = nullptr);
    ~LocalGameDialog() override;

    LocalGameConfig config() const;

   public slots:
    void on_btnStartGame_clicked();
    void on_btnCancel_clicked();
    void on_spinBox_numPlayers_valueChanged(int);
    void on_comboBox_gameMap_currentIndexChanged(int);

   private:
    void populateAvailableMaps();

    Ui::LocalGameDialog* ui;
};
