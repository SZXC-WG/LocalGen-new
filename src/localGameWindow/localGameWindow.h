#ifndef LOCALGAMEWINDOW_H
#define LOCALGAMEWINDOW_H

#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QPaintEvent>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <functional>
#include <vector>

class QResizeEvent;

#include "../GameEngine/player.hpp"
#include "../localGameDialog/localGameDialog.h"
#include "../mapWidget/mapWidget.h"

namespace game {
class BasicGame;
struct RankItem;
}  // namespace game

struct LeaderboardRow {
    index_t playerId = -1;
    QString playerName;
    army_t army = 0;
    int land = 0;
    QColor playerColor;
};

class FloatingLeaderboardWidget : public QWidget {
   public:
    struct Column {
        QString title;
        int width = 80;
        int minWidth = 0;
        int maxWidth = 0;
        std::function<QString(const LeaderboardRow&)> textProvider;
        std::function<QColor(const LeaderboardRow&)> backgroundProvider;
        std::function<QColor(const LeaderboardRow&)> foregroundProvider;
    };

    explicit FloatingLeaderboardWidget(QWidget* parent = nullptr);

    void setColumns(std::vector<Column> columns);
    void setRows(std::vector<LeaderboardRow> rows);

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    std::vector<int> computeColumnWidths() const;
    void updateFixedSize();

    std::vector<Column> columns;
    std::vector<LeaderboardRow> rows;
    constexpr static int headerHeight = 30;
    constexpr static int rowHeight = 28;
    constexpr static int horizontalPadding = 12;
};

// Helper class
class HumanPlayer : public Player {
   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override;
    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& rank) override;
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
    void updateLeaderboard(const std::vector<game::RankItem>& rank);
    void runHalfTurn();
    void scheduleNextHalfTurn(double delayMs);
    void stopGameLoop();
    void positionFloatingWidgets();

   protected:
    void resizeEvent(QResizeEvent* event) override;

    MapWidget* gameMap = nullptr;
    FloatingLeaderboardWidget* leaderboardWidget = nullptr;
    HumanPlayer* humanPlayer = nullptr;
    game::BasicGame* game = nullptr;
    QTimer* halfTurnTimer = nullptr;
    QLabel* turnLabel = nullptr;
    bool gameRunning = false;
    double halfTurnDurationMs = 500.0;
};

#endif  // LOCALGAMEWINDOW_H
