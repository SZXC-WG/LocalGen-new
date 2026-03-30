#ifndef LOCALGAMEWINDOW_H
#define LOCALGAMEWINDOW_H

#include <QDialog>
#include <QFrame>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QPointF>
#include <QPushButton>
#include <QResizeEvent>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QValueAxis>
#include <vector>

#include "../leaderboardWidget/leaderboardWidget.h"
#include "../localGameDialog/localGameDialog.h"
#include "../mapWidget/mapWidget.h"
#include "core/game.hpp"
#include "core/player.hpp"

struct PlayerAnalysisSeries {
    QLineSeries* series = nullptr;
    QList<QPointF> linearArmyHistory;
    QList<QPointF> linearLandHistory;
    QList<QPointF> logArmyHistory;
    QList<QPointF> logLandHistory;
};

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
    void initializeAnalysisWidget();
    void refreshAnalysisChart();
    void updateAnalysisAxisRanges();
    void updateAnalysis(const std::vector<LeaderboardRow>& rows);
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
    QFrame* analysisWidget = nullptr;
    QChart* analysisChart = nullptr;
    QChartView* analysisChartView = nullptr;
    QValueAxis* analysisAxisX = nullptr;
    QValueAxis* analysisAxisYLinear = nullptr;
    QLogValueAxis* analysisAxisYLog = nullptr;
    QPushButton* analysisMetricToggle = nullptr;
    QPushButton* analysisScaleToggle = nullptr;
    HumanPlayer* humanPlayer = nullptr;
    BasicGame* game = nullptr;
    QTimer* halfTurnTimer = nullptr;
    QLabel* turnLabel = nullptr;
    bool gameRunning = false;
    bool analysisEnabled = false;
    bool analysisShowingLand = false;
    bool analysisUsingLogScale = false;
    double halfTurnDurationMs = 500.0;
    int analysisSampleCount = 0;
    army_t analysisArmyMax = 0;
    int analysisLandMax = 0;
    std::vector<PlayerAnalysisSeries> analysisSeries;

    index_t humanPlayerId = -2;
    int generalRow = -1, generalCol = -1;
};

#endif  // LOCALGAMEWINDOW_H
