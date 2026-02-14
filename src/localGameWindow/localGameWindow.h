#ifndef LOCALGAMEWINDOW_H
#define LOCALGAMEWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <functional>

#include "../GameEngine/player.hpp"
#include "../localGameDialog/localGameDialog.h"
#include "../mapWidget/mapWidget.h"

namespace game {
class BasicGame;
}

// Helper class
class HumanPlayer : public Player {
   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override;
    void requestMove(const BoardView& boardView) override;
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
    void runHalfTurn();
    void scheduleNextHalfTurn(double delayMs);
    void stopGameLoop();

    MapWidget* gameMap = nullptr;
    HumanPlayer* humanPlayer = nullptr;
    game::BasicGame* game = nullptr;
    QTimer* halfTurnTimer = nullptr;
    QLabel* turnLabel = nullptr;
    bool gameRunning = false;
    double halfTurnDurationMs = 500.0;
};

#endif  // LOCALGAMEWINDOW_H
