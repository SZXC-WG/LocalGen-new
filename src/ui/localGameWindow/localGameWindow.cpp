#include "localGameWindow.h"

#include <QElapsedTimer>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QtCharts/QLegend>
#include <algorithm>
#include <cmath>

#include "core/bot.h"
#include "core/game.hpp"
#include "core/map.hpp"

FloatingLeaderboardWidget::FloatingLeaderboardWidget(QWidget* parent)
    : QWidget(parent) {
    setFont(QFont("Quicksand", 10, QFont::Medium));
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void FloatingLeaderboardWidget::setColumns(std::vector<Column> columns) {
    this->columns = std::move(columns);
    updateFixedSize();
    update();
}

void FloatingLeaderboardWidget::setRows(std::vector<LeaderboardRow> rows) {
    this->rows = std::move(rows);
    updateFixedSize();
    update();
}

void FloatingLeaderboardWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (columns.empty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen borderPen(Qt::black);
    borderPen.setWidth(2);
    painter.setPen(borderPen);

    std::vector<int> columnWidths = computeColumnWidths();

    QFont headerFont = font();
    headerFont.setBold(true);

    int x = 0;
    for (size_t i = 0; i < columns.size(); ++i) {
        const auto& column = columns[i];
        QRect cellRect(x, 0, columnWidths[i], headerHeight);
        painter.fillRect(cellRect, Qt::white);
        painter.drawRect(cellRect);
        painter.setFont(headerFont);
        painter.setPen(Qt::black);
        painter.drawText(cellRect, Qt::AlignCenter, column.title);
        painter.setPen(borderPen);
        x += columnWidths[i];
    }

    QFont bodyFont = font();
    bodyFont.setBold(false);
    painter.setFont(bodyFont);
    QFontMetrics metrics(bodyFont);

    int y = headerHeight;
    for (const auto& row : rows) {
        int rowX = 0;
        for (size_t i = 0; i < columns.size(); ++i) {
            const auto& column = columns[i];
            QRect cellRect(rowX, y, columnWidths[i], rowHeight);
            QColor background = Qt::white;
            QColor foreground = Qt::black;
            QString text;

            if (column.backgroundProvider) {
                background = column.backgroundProvider(row);
            }
            if (column.foregroundProvider) {
                foreground = column.foregroundProvider(row);
            }
            if (column.textProvider) {
                text = column.textProvider(row);
            }

            painter.fillRect(cellRect, background);
            painter.setPen(borderPen);
            painter.drawRect(cellRect);
            painter.setPen(foreground);

            QString displayText = metrics.elidedText(
                text, Qt::ElideRight, cellRect.width() - horizontalPadding);
            painter.drawText(cellRect, Qt::AlignCenter, displayText);

            painter.setPen(borderPen);
            rowX += columnWidths[i];
        }

        if (!row.isAlive) {
            painter.fillRect(QRect(0, y, width(), rowHeight),
                             QColor(0, 0, 0, 153));
        }

        y += rowHeight;
    }

    QPen outerBorderPen(Qt::black);
    outerBorderPen.setWidth(3);
    painter.setPen(outerBorderPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
}

std::vector<int> FloatingLeaderboardWidget::computeColumnWidths() const {
    std::vector<int> widths;
    widths.reserve(columns.size());

    QFontMetrics metrics(font());
    for (const auto& column : columns) {
        int width = column.width;
        if (width <= 0) {
            int maxTextWidth = metrics.horizontalAdvance(column.title);
            if (column.textProvider) {
                for (const auto& row : rows) {
                    maxTextWidth = std::max(
                        maxTextWidth,
                        metrics.horizontalAdvance(column.textProvider(row)));
                }
            }
            width = maxTextWidth + horizontalPadding * 2;
        }

        if (column.minWidth > 0) {
            width = std::max(width, column.minWidth);
        }
        if (column.maxWidth > 0) {
            width = std::min(width, column.maxWidth);
        }
        widths.push_back(width);
    }

    return widths;
}

void FloatingLeaderboardWidget::updateFixedSize() {
    int totalWidth = 0;
    for (int columnWidth : computeColumnWidths()) {
        totalWidth += columnWidth;
    }
    int height = headerHeight + static_cast<int>(rows.size()) * rowHeight;
    setFixedSize(totalWidth, height);
}

void HumanPlayer::init(index_t playerId, const GameConstantsPack& constants) {
    this->playerId = playerId;
}

void HumanPlayer::requestMove(const BoardView& boardView,
                              const std::vector<RankItem>& rank) {
    if (boardViewHandler) {
        boardViewHandler(boardView);
    }
}

void HumanPlayer::setBoardViewHandler(
    std::function<void(const BoardView&)> boardViewHandler) {
    this->boardViewHandler = std::move(boardViewHandler);
}

namespace {

inline QColor playerColor(index_t playerId) {
    static const QColor palette[16] = {
        "#FF0000", "#2792FF", "#008000", "#008080", "#FF7010", "#F032E6",
        "#800080", "#9B0101", "#B3AC32", "#9A5E24", "#1031FF", "#594CA5",
        "#85A91C", "#F87375", "#B47FCA", "#B49971"};
    return palette[playerId % 16];
}

double adjustedAxisMax(double value) {
    if (value <= 1.0) {
        return 1.0;
    }

    return value + std::max(1.0, value * 0.08);
}

void styleAnalysisAxis(QAbstractAxis* axis, const QColor& foreground,
                       const QColor& gridColor) {
    if (axis == nullptr) {
        return;
    }

    axis->setLabelsFont(QFont("Quicksand", 9, QFont::Medium));
    axis->setTitleFont(QFont("Quicksand", 10, QFont::DemiBold));
    axis->setLabelsColor(foreground);
    axis->setTitleBrush(QBrush(foreground));
    axis->setLinePenColor(foreground);
    axis->setGridLineColor(gridColor);
    axis->setMinorGridLineColor(gridColor);
}

QPushButton* createAnalysisToggleButton(const QString& text, QWidget* parent) {
    QPushButton* button = new QPushButton(text, parent);
    button->setCheckable(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(240, 244, 248, 28);"
        "color: rgb(240, 244, 248);"
        "border: 1px solid rgba(240, 244, 248, 110);"
        "border-radius: 14px;"
        "padding: 5px 12px;"
        "font: 600 10pt 'Quicksand';"
        "}"
        "QPushButton:checked {"
        "background-color: rgba(73, 160, 255, 90);"
        "}");
    return button;
}

qreal smoothedLinearValue(const QList<QPointF>& history, qreal rawValue) {
    constexpr qreal smoothingAlpha = 0.65;
    if (history.isEmpty() || rawValue <= 0.0) {
        return rawValue;
    }
    return history.constLast().y() +
           smoothingAlpha * (rawValue - history.constLast().y());
}

qreal smoothedLogValue(const QList<QPointF>& history, qreal rawValue) {
    constexpr qreal smoothingAlpha = 0.65;
    const qreal rawLogValue = std::log10(std::max<qreal>(1.0, rawValue));
    if (history.isEmpty() || rawValue <= 0.0) {
        return std::pow(static_cast<qreal>(10.0), rawLogValue);
    }
    const qreal previousSmoothedLog =
        std::log10(std::max<qreal>(1.0, history.constLast().y()));
    const qreal smoothedLog =
        previousSmoothedLog +
        smoothingAlpha * (rawLogValue - previousSmoothedLog);
    return std::pow(static_cast<qreal>(10.0), smoothedLog);
}

void appendAnalysisPoint(QList<QPointF>& history, qreal x, qreal y) {
    history.append(QPointF(x, y));
}

void appendAnalysisHistories(PlayerAnalysisSeries& series, qreal stepValue,
                             qreal armyValue, qreal landValue) {
    appendAnalysisPoint(
        series.linearArmyHistory, stepValue,
        smoothedLinearValue(series.linearArmyHistory, armyValue));
    appendAnalysisPoint(
        series.linearLandHistory, stepValue,
        smoothedLinearValue(series.linearLandHistory, landValue));
    appendAnalysisPoint(series.logArmyHistory, stepValue,
                        smoothedLogValue(series.logArmyHistory, armyValue));
    appendAnalysisPoint(series.logLandHistory, stepValue,
                        smoothedLogValue(series.logLandHistory, landValue));
}

const QList<QPointF>& analysisHistoryForMode(const PlayerAnalysisSeries& series,
                                             bool showLand, bool useLogScale) {
    if (useLogScale) {
        return showLand ? series.logLandHistory : series.logArmyHistory;
    }
    return showLand ? series.linearLandHistory : series.linearArmyHistory;
}

QChart* createAnalysisChart(const QString& title, QValueAxis*& axisX) {
    const QColor panelBackground(20, 23, 28, 0);
    const QColor plotBackground(32, 37, 44, 160);
    const QColor foreground(240, 244, 248);
    const QColor gridColor(240, 244, 248, 64);

    QChart* chart = new QChart();
    chart->setTitle(title);
    chart->setTitleFont(QFont("Quicksand", 11, QFont::Bold));
    chart->setTitleBrush(QBrush(foreground));
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setBackgroundBrush(QBrush(panelBackground));
    chart->setBackgroundPen(Qt::NoPen);
    chart->setBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(plotBackground));
    chart->setPlotAreaBackgroundPen(QPen(gridColor));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setMargins(QMargins(10, 10, 10, 10));
    chart->legend()->hide();

    axisX = new QValueAxis(chart);
    axisX->setTitleText("Step");
    axisX->setLabelFormat("%.0f");
    axisX->setTickCount(6);
    axisX->setMinorTickCount(0);
    axisX->setRange(0.0, 1.0);
    styleAnalysisAxis(axisX, foreground, gridColor);
    chart->addAxis(axisX, Qt::AlignBottom);

    return chart;
}

inline DisplayTile toDisplayTile(const TileView& tile) {
    DisplayTile display;
    display.type = tile.type;
    display.lightIcon = false;
    if (!tile.visible) {
        if (tile.type == TILE_SWAMP)
            display.color.setRgb(128, 128, 128);
        else
            display.color.setRgb(57, 57, 57);
        display.text.clear();
        display.displayBorders = false;
        return display;
    }
    if (tile.occupier >= 0) {
        display.color = playerColor(tile.occupier);
    } else {
        switch (tile.type) {
            case TILE_MOUNTAIN:
            case TILE_LOOKOUT:
            case TILE_OBSERVATORY:
            case TILE_OBSTACLE:    display.color.setRgb(187, 187, 187); break;
            case TILE_SWAMP:
            case TILE_CITY:        display.color.setRgb(128, 128, 128); break;
            default:               display.color.setRgb(220, 220, 220); break;
        }
    }
    if (tile.army != 0 || tile.type == TILE_CITY) {
        display.text = QString::number(tile.army);
        if (tile.occupier == -1 && tile.type == TILE_NEUTRAL) {
            display.color.setRgb(128, 128, 128);
        }
    } else {
        display.text.clear();
    }
    return display;
}

}  // namespace

LocalGameWindow::LocalGameWindow(QWidget* parent, const LocalGameConfig& config)
    : QDialog(parent) {
    // Present the gameplay UI as a regular top-level window to keep it on the
    // current virtual desktop.
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::Window;
    flags |= Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint;
    flags &= ~Qt::Dialog;
    setWindowFlags(flags);

    setWindowTitle("Local Game");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);

    Board initialBoard;
    if (config.mapFilePath.isEmpty()) {
        initialBoard = Board::generate(config.mapWidth, config.mapHeight);
    } else {
        QString errMsg;
        MapDocument mapDoc = openMap_v6(config.mapFilePath, errMsg);
        if (!errMsg.isEmpty()) {
            QMessageBox::critical(
                this, "Local Game",
                QString("Failed to load the selected map.\n%1").arg(errMsg));
            return;
        }
        initialBoard = mapDoc.board;
    }

    if (initialBoard.getWidth() <= 0 || initialBoard.getHeight() <= 0) {
        QMessageBox::critical(this, "Local Game",
                              QString("The selected map is empty or invalid."));
        return;
    }

    gameMap = new MapWidget(this, initialBoard.getWidth(),
                            initialBoard.getHeight(), true, 25);
    halfTurnTimer = new QTimer(this);
    halfTurnTimer->setSingleShot(true);
    halfTurnTimer->setTimerType(Qt::PreciseTimer);
    connect(halfTurnTimer, &QTimer::timeout, this,
            &LocalGameWindow::runHalfTurn);

    halfTurnDurationMs =
        500.0 / static_cast<double>(std::max(config.gameSpeed, 1));
    analysisEnabled = config.showAnalysis;

    std::vector<Player*> players;
    std::vector<index_t> teams;
    std::vector<std::string> names;

    players.reserve(config.players.size() + 1);
    teams.reserve(config.players.size() + 1);
    names.reserve(config.players.size() + 1);

    for (QString name : config.players) {
        if (name == "Human") {
            humanPlayer = new HumanPlayer();
            humanPlayer->setBoardViewHandler(
                [this](const BoardView& boardView) { updateView(boardView); });
            players.push_back(humanPlayer);
            teams.push_back(static_cast<index_t>(teams.size()));
            names.push_back("Human");
            continue;
        }
        std::string stdName = name.toStdString();
        BasicBot* bot = BotFactory::instance().create(stdName);
        if (bot == nullptr) {
            continue;
        }
        players.push_back(bot);
        teams.push_back(static_cast<index_t>(teams.size()));
        names.push_back(stdName);
    }

    game = new BasicGame(true, players, teams, names, initialBoard);
    const int initResult = game->init();
    if (initResult != 0) {
        QString errMsg = "Failed to initialize local game.";
        if (initResult == 1) {
            errMsg = QString(
                         "The selected map does not have enough spawn points "
                         "or blank tiles to accommodate %1 players.")
                         .arg(static_cast<int>(config.players.size()));
        }
        QMessageBox::critical(this, "Local Game", errMsg);
        delete game;
        game = nullptr;
        humanPlayer = nullptr;
        return;
    }

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(gameMap);
    setLayout(layout);
    resize(800, 600);

    turnLabel = new QLabel("Turn 0", this);
    turnLabel->setFont(QFont("Quicksand", 18, QFont::Bold));
    turnLabel->setStyleSheet(
        "QLabel { background-color: white; "
        "color: black; padding: 8px 16px; "
        "font-size: 18px; font-weight: bold; "
        "border-radius: 4px; }");
    turnLabel->setMinimumWidth(120);
    turnLabel->setAlignment(Qt::AlignCenter);
    turnLabel->move(10, 10);
    turnLabel->raise();

    if (analysisEnabled) {
        initializeAnalysisWidget();
    }

    leaderboardWidget = new FloatingLeaderboardWidget(this);
    leaderboardWidget->setColumns(
        {{"Player", 0, 140, 260,
          [](const LeaderboardRow& row) { return row.playerName; },
          [](const LeaderboardRow& row) { return row.playerColor; },
          [](const LeaderboardRow&) { return QColor(Qt::white); }},
         {"Army", 72, 0, 0,
          [](const LeaderboardRow& row) { return QString::number(row.army); },
          [](const LeaderboardRow&) { return QColor(Qt::white); },
          [](const LeaderboardRow&) { return QColor(Qt::black); }},
         {"Land", 72, 0, 0,
          [](const LeaderboardRow& row) { return QString::number(row.land); },
          [](const LeaderboardRow&) { return QColor(Qt::white); },
          [](const LeaderboardRow&) { return QColor(Qt::black); }}});

    if (humanPlayer != nullptr) {
        gameMap->bindMoveQueue(humanPlayer->getMoveQueue());
        humanPlayerId = humanPlayer->playerId;
    }

    if (game != nullptr) {
        updateLeaderboard(game->ranklist());
        positionFloatingWidgets();
        gameRunning = true;
        scheduleNextHalfTurn(0.0);
    }
}

LocalGameWindow::~LocalGameWindow() {
    stopGameLoop();
    if (game != nullptr) {
        delete game;
        game = nullptr;
        humanPlayer = nullptr;
    }
}

void LocalGameWindow::initializeAnalysisWidget() {
    if (!analysisEnabled || game == nullptr || analysisWidget != nullptr) {
        return;
    }

    const QColor foreground(240, 244, 248);
    const QColor gridColor(240, 244, 248, 64);

    analysisWidget = new QFrame(this);
    analysisWidget->setFocusPolicy(Qt::NoFocus);
    analysisWidget->setObjectName("analysisWidget");
    analysisWidget->setStyleSheet(
        "QFrame#analysisWidget {"
        "background-color: rgba(18, 21, 26, 232);"
        "border: 2px solid rgba(240, 244, 248, 180);"
        "border-radius: 6px;"
        "}");

    QVBoxLayout* analysisLayout = new QVBoxLayout(analysisWidget);
    analysisLayout->setContentsMargins(8, 8, 8, 8);
    analysisLayout->setSpacing(6);

    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->addStretch(1);

    analysisMetricToggle =
        createAnalysisToggleButton("Switch to Land", analysisWidget);
    controlsLayout->addWidget(analysisMetricToggle);

    analysisScaleToggle =
        createAnalysisToggleButton("Switch to Log", analysisWidget);
    controlsLayout->addWidget(analysisScaleToggle);
    analysisLayout->addLayout(controlsLayout);

    analysisChart = createAnalysisChart("Army Trend", analysisAxisX);

    analysisAxisYLinear = new QValueAxis(analysisChart);
    analysisAxisYLinear->setLabelFormat("%.0f");
    analysisAxisYLinear->setTickCount(6);
    analysisAxisYLinear->setMinorTickCount(0);
    analysisAxisYLinear->setRange(0.0, 1.0);
    styleAnalysisAxis(analysisAxisYLinear, foreground, gridColor);
    analysisChart->addAxis(analysisAxisYLinear, Qt::AlignLeft);

    analysisAxisYLog = new QLogValueAxis(analysisChart);
    analysisAxisYLog->setLabelFormat("%.0f");
    analysisAxisYLog->setBase(10.0);
    analysisAxisYLog->setMinorTickCount(8);
    analysisAxisYLog->setRange(1.0, 10.0);
    styleAnalysisAxis(analysisAxisYLog, foreground, gridColor);

    analysisChartView = new QChartView(analysisChart, analysisWidget);
    analysisChartView->setFocusPolicy(Qt::NoFocus);
    analysisChartView->setFrameShape(QFrame::NoFrame);
    analysisChartView->setRubberBand(QChartView::NoRubberBand);
    analysisChartView->setRenderHint(QPainter::Antialiasing, true);
    analysisChartView->setStyleSheet("background: transparent;");
    analysisLayout->addWidget(analysisChartView, 1);

    connect(analysisMetricToggle, &QPushButton::toggled, this,
            [this](bool checked) {
                analysisShowingLand = checked;
                refreshAnalysisChart();
            });
    connect(analysisScaleToggle, &QPushButton::toggled, this,
            [this](bool checked) {
                analysisUsingLogScale = checked;
                refreshAnalysisChart();
            });

    analysisSeries.resize(static_cast<size_t>(game->getPlayerCount()));
    for (index_t playerId = 0; playerId < game->getPlayerCount(); ++playerId) {
        const QColor color = playerColor(playerId);
        const QString name = QString::fromStdString(game->getName(playerId));

        QPen pen(color);
        pen.setWidth(2);

        QLineSeries* series = new QLineSeries(analysisChart);
        series->setName(name);
        series->setPen(pen);
        analysisChart->addSeries(series);
        series->attachAxis(analysisAxisX);
        series->attachAxis(analysisAxisYLinear);

        analysisSeries[static_cast<size_t>(playerId)].series = series;
    }

    refreshAnalysisChart();
}

void LocalGameWindow::refreshAnalysisChart() {
    if (!analysisEnabled || analysisChart == nullptr) {
        return;
    }

    const bool showLand = analysisShowingLand;
    const bool useLogScale = analysisUsingLogScale;
    analysisChart->setTitle(
        QString("%1 Trend (%2)")
            .arg(showLand ? "Land" : "Army", useLogScale ? "Log" : "Linear"));

    if (analysisMetricToggle != nullptr) {
        analysisMetricToggle->setText(showLand ? "Switch to Army"
                                               : "Switch to Land");
    }
    if (analysisScaleToggle != nullptr) {
        analysisScaleToggle->setText(useLogScale ? "Switch to Linear"
                                                 : "Switch to Log");
    }

    QAbstractAxis* activeAxis =
        useLogScale ? static_cast<QAbstractAxis*>(analysisAxisYLog)
                    : static_cast<QAbstractAxis*>(analysisAxisYLinear);
    QAbstractAxis* inactiveAxis =
        useLogScale ? static_cast<QAbstractAxis*>(analysisAxisYLinear)
                    : static_cast<QAbstractAxis*>(analysisAxisYLog);

    for (PlayerAnalysisSeries& playerSeries : analysisSeries) {
        if (playerSeries.series != nullptr && inactiveAxis != nullptr) {
            playerSeries.series->detachAxis(inactiveAxis);
        }
    }
    if (inactiveAxis != nullptr &&
        analysisChart->axes(Qt::Vertical).contains(inactiveAxis)) {
        analysisChart->removeAxis(inactiveAxis);
    }
    if (activeAxis != nullptr &&
        !analysisChart->axes(Qt::Vertical).contains(activeAxis)) {
        analysisChart->addAxis(activeAxis, Qt::AlignLeft);
    }

    updateAnalysisAxisRanges();

    for (PlayerAnalysisSeries& playerSeries : analysisSeries) {
        if (playerSeries.series == nullptr) {
            continue;
        }
        if (activeAxis != nullptr) {
            playerSeries.series->attachAxis(activeAxis);
        }
        playerSeries.series->replace(
            analysisHistoryForMode(playerSeries, showLand, useLogScale));
    }
}

void LocalGameWindow::updateAnalysisAxisRanges() {
    if (!analysisEnabled || analysisChart == nullptr) {
        return;
    }

    const qreal axisMaxX = std::max<qreal>(
        1.0, static_cast<qreal>(std::max(0, analysisSampleCount - 1)));
    if (analysisAxisX != nullptr) {
        analysisAxisX->setRange(0.0, axisMaxX);
    }

    const double axisMaxY =
        analysisShowingLand
            ? adjustedAxisMax(static_cast<double>(analysisLandMax))
            : adjustedAxisMax(static_cast<double>(analysisArmyMax));
    if (analysisUsingLogScale && analysisAxisYLog != nullptr) {
        analysisAxisYLog->setRange(1.0, std::max(10.0, axisMaxY));
    } else if (!analysisUsingLogScale && analysisAxisYLinear != nullptr) {
        analysisAxisYLinear->setRange(0.0, axisMaxY);
    }
}

void LocalGameWindow::updateAnalysis(const std::vector<LeaderboardRow>& rows) {
    if (!analysisEnabled || analysisWidget == nullptr) {
        return;
    }

    const qreal stepValue = static_cast<qreal>(analysisSampleCount);
    army_t maxArmy = analysisArmyMax;
    int maxLand = analysisLandMax;

    std::vector<bool> updated(analysisSeries.size(), false);
    for (const LeaderboardRow& row : rows) {
        if (row.playerId < 0 ||
            static_cast<size_t>(row.playerId) >= analysisSeries.size()) {
            continue;
        }

        PlayerAnalysisSeries& series =
            analysisSeries[static_cast<size_t>(row.playerId)];
        appendAnalysisHistories(series, stepValue, static_cast<qreal>(row.army),
                                static_cast<qreal>(row.land));

        maxArmy = std::max(maxArmy, row.army);
        maxLand = std::max(maxLand, row.land);
        updated[static_cast<size_t>(row.playerId)] = true;
    }

    for (size_t i = 0; i < analysisSeries.size(); ++i) {
        if (updated[i]) {
            continue;
        }
        appendAnalysisHistories(analysisSeries[i], stepValue, 0.0, 0.0);
    }

    ++analysisSampleCount;
    analysisArmyMax = maxArmy;
    analysisLandMax = maxLand;
    updateAnalysisAxisRanges();

    for (PlayerAnalysisSeries& playerSeries : analysisSeries) {
        if (playerSeries.series == nullptr) {
            continue;
        }
        const QList<QPointF>& history = analysisHistoryForMode(
            playerSeries, analysisShowingLand, analysisUsingLogScale);
        if (!history.isEmpty()) {
            playerSeries.series->append(history.constLast());
        }
    }
}

void LocalGameWindow::updateView(const BoardView& boardView) {
    int height = gameMap->mapHeight();
    int width = gameMap->mapWidth();
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            const TileView& tileView = boardView.tileAt(r + 1, c + 1);
            gameMap->tileAt(r, c) = toDisplayTile(tileView);
            if (tileView.type == TILE_GENERAL &&
                tileView.occupier == humanPlayerId) {
                generalRow = r, generalCol = c;
            }
        }
    }
    gameMap->update();
}

void LocalGameWindow::runHalfTurn() {
    if (!gameRunning || game == nullptr) {
        return;
    }
    if (game->getAlivePlayers().size() <= 1) {
        stopGameLoop();
        return;
    }

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    game->step();
    if (!game->isAlive(humanPlayerId) ||
        static_cast<int>(game->getAlivePlayers().size()) <= 1) {
        if (humanPlayer != nullptr) {
            humanPlayer = nullptr;
            gameMap->bindMoveQueue(nullptr);
        }
        updateView(game->fullView());
    }
    updateLeaderboard(game->ranklist());

    turn_t curTurn = game->getCurTurn();
    uint8_t curHalfTurnPhase = game->getHalfTurnPhase();
    if (curHalfTurnPhase == 0) {
        turnLabel->setText(QString("Turn %1.").arg(curTurn - 1));
    } else {
        turnLabel->setText(QString("Turn %1").arg(curTurn));
    }

    double elapsedMs = static_cast<double>(elapsedTimer.nsecsElapsed()) / 1e6;

    if (game->getAlivePlayers().size() <= 1) {
        stopGameLoop();
        return;
    }

    double waitMs = halfTurnDurationMs - elapsedMs;
    scheduleNextHalfTurn(waitMs > 0.0 ? waitMs : 0.0);
}

void LocalGameWindow::scheduleNextHalfTurn(double delayMs) {
    if (!gameRunning || halfTurnTimer == nullptr) {
        return;
    }
    int waitMs = static_cast<int>(std::lround(delayMs));
    if (waitMs < 0) waitMs = 0;
    halfTurnTimer->start(waitMs);
}

void LocalGameWindow::stopGameLoop() {
    gameRunning = false;
    if (halfTurnTimer != nullptr && halfTurnTimer->isActive()) {
        halfTurnTimer->stop();
    }
    if (gameMap != nullptr) {
        gameMap->bindMoveQueue(nullptr);
        gameMap->setFocusEnabled(false);
    }
}

void LocalGameWindow::updateLeaderboard(const std::vector<RankItem>& rank) {
    if (leaderboardWidget == nullptr || game == nullptr) {
        return;
    }

    std::vector<LeaderboardRow> rows;
    rows.reserve(rank.size());
    for (const auto& item : rank) {
        LeaderboardRow row;
        row.playerId = item.player;
        row.playerName = QString::fromStdString(game->getName(item.player));
        row.army = item.army, row.land = item.land;
        row.playerColor = playerColor(item.player);
        row.isAlive = game->isAlive(item.player);
        rows.push_back(std::move(row));
    }

    updateAnalysis(rows);
    leaderboardWidget->setRows(std::move(rows));
    positionFloatingWidgets();
}

void LocalGameWindow::positionFloatingWidgets() {
    const int margin = 6;

    if (turnLabel != nullptr) {
        turnLabel->move(margin, margin);
        turnLabel->raise();
    }

    if (analysisWidget != nullptr) {
        const int analysisWidth = std::clamp(width() / 3, 340, 560);
        const int analysisHeight = std::clamp(height() / 2, 340, 520);
        const int x = std::max(margin, width() - analysisWidth - margin);
        const int y = std::max(margin, height() - analysisHeight - margin);
        analysisWidget->setGeometry(x, y, analysisWidth, analysisHeight);
        analysisWidget->raise();
    }

    if (leaderboardWidget != nullptr) {
        int x = width() - leaderboardWidget->width() - margin;
        if (x < margin) {
            x = margin;
        }
        leaderboardWidget->move(x, margin);
        leaderboardWidget->raise();
    }

    if (turnLabel != nullptr) turnLabel->raise();
}

void LocalGameWindow::keyPressEvent(QKeyEvent* event) {
    int key = event->key();
    if (gameRunning && game->isAlive(humanPlayerId) && generalRow != -1) {
        if (key == Qt::Key_Escape) {
            const QMessageBox::StandardButton result = QMessageBox::question(
                this, "Surrender",
                "Are you sure you want to surrender? You will lose control of "
                "your armies and reveal the full map.",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (gameRunning && game->isAlive(humanPlayerId) &&
                result == QMessageBox::Yes) {
                auto moveQueue = humanPlayer->getMoveQueue();
                moveQueue->clear();
                moveQueue->emplace_back(MoveType::SURRENDER);
            }
            return;
        }
        if (key == Qt::Key_H) {
            gameMap->setFocusCell(generalRow, generalCol);
            return;
        }
        if (key == Qt::Key_G) {
            gameMap->centerOnCell(generalRow, generalCol);
            return;
        }
    }
    QDialog::keyPressEvent(event);
}

void LocalGameWindow::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    positionFloatingWidgets();
}
