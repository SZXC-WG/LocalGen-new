// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALGAMEWINDOW_H
#define LOCALGAMEWINDOW_H

#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <vector>

#include "../analysisChartWidget/analysisChartWidget.h"
#include "../leaderboardWidget/leaderboardWidget.h"
#include "../localGameDialog/localGameDialog.h"
#include "../mapWidget/mapWidget.h"
#include "core/game.hpp"
#include "core/player.hpp"

// Helper class
class HumanPlayer : public Player {
   public:
    void init(index_t playerId, const GameConstantsPack& constants) override;
    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override;
    std::deque<Move>* getMoveQueue() { return &moveQueue; }
    void setBoardViewHandler(
        std::function<void(const BoardView&)> boardViewHandler);

    index_t playerId = -1;

   private:
    std::function<void(const BoardView&)> boardViewHandler;
};

class LocalGameWindow : public QDialog {
    Q_OBJECT

   public:
    explicit LocalGameWindow(QWidget* parent, const LocalGameConfig& config);
    ~LocalGameWindow();

   private:
    void updateView(const BoardView& boardView);
    void updateLeaderboard(const std::vector<RankItem>& rank);
    void runHalfTurn();
    void scheduleNextHalfTurn(double delayMs);
    void stopGameLoop();
    void positionFloatingWidgets();

   protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    MapWidget* gameMap = nullptr;
    LeaderboardWidget* leaderboardWidget = nullptr;
    AnalysisChartWidget* analysisChartWidget = nullptr;
    HumanPlayer* humanPlayer = nullptr;
    BasicGame* game = nullptr;
    QTimer* halfTurnTimer = nullptr;
    QLabel* turnLabel = nullptr;
    bool gameRunning = false;
    bool analysisEnabled = false;
    double halfTurnDurationMs = 500.0;

    index_t humanPlayerId = -2;
    int generalRow = -1, generalCol = -1;
};

#endif  // LOCALGAMEWINDOW_H
