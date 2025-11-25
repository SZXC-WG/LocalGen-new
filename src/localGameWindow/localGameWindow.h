#ifndef LOCALGAMEWINDOW_H
#define LOCALGAMEWINDOW_H

#include <QDialog>

#include "../GameEngine/player.h"
#include "../localGameDialog/localGameDialog.h"
#include "../mapWidget/mapWidget.h"

// Helper class
class HumanPlayer : public Player {
   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override;
    void requestMove(BoardView& boardView) override;
    std::deque<Move>* getMoveQueue() { return &moveQueue; }
};

class LocalGameWindow : public QDialog {
    Q_OBJECT

   public:
    explicit LocalGameWindow(QWidget* parent, const LocalGameConfig& config);
    ~LocalGameWindow();

   private:
    MapWidget* gameMap;
    HumanPlayer* humanPlayer;
};

#endif  // LOCALGAMEWINDOW_H
