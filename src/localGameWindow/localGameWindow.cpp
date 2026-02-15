#include "localGameWindow.h"

#include <QElapsedTimer>
#include <QFont>
#include <QFontMetrics>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

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

void HumanPlayer::init(index_t playerId,
                       const game::GameConstantsPack& constants) {
    this->playerId = playerId;
}

void HumanPlayer::requestMove(const BoardView& boardView,
                              const std::vector<game::RankItem>& rank) {
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

inline DisplayTile toDisplayTile(const TileView& tile) {
    DisplayTile display;
    display.type = tile.type;
    display.lightIcon = false;
    if (!tile.visible) {
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

InitBoard createRandomBoard(int width, int height) {
    InitBoard board(height, width);

    QRandomGenerator* rng = QRandomGenerator::global();
    int area = width * height;
    int numMountains = rng->bounded(area / 8, area / 8 + area / 20 + 1);
    for (int i = 0; i < numMountains; ++i) {
        Coord pos(rng->bounded(height) + 1, rng->bounded(width) + 1);
        board.changeTile(pos, Tile(-1, TILE_MOUNTAIN, 0));
    }

    int numCities = rng->bounded(area / 30, area / 15 + 1);
    for (int i = 0; i < numCities; ++i) {
        Coord pos(rng->bounded(height) + 1, rng->bounded(width) + 1);
        army_t army = static_cast<army_t>(rng->bounded(40, 50));
        board.changeTile(pos, Tile(-1, TILE_CITY, army));
    }

    return board;
}

}  // namespace

LocalGameWindow::LocalGameWindow(QWidget* parent, const LocalGameConfig& config)
    : QDialog(parent) {
    setWindowTitle("Local Game");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);

    gameMap = new MapWidget(this, config.mapWidth, config.mapHeight, true, 25);
    halfTurnTimer = new QTimer(this);
    halfTurnTimer->setSingleShot(true);
    halfTurnTimer->setTimerType(Qt::PreciseTimer);
    connect(halfTurnTimer, &QTimer::timeout, this,
            &LocalGameWindow::runHalfTurn);

    halfTurnDurationMs =
        500.0 / static_cast<double>(std::max(config.gameSpeed, 1));

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

    InitBoard initialBoard =
        createRandomBoard(config.mapWidth, config.mapHeight);
    game = new game::BasicGame(true, players, teams, names, initialBoard);
    if (game->init() != 0) {
        QMessageBox::critical(this, "Local Game",
                              "Failed to initialize local game.");
        delete game;
        game = nullptr;
    }

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(gameMap);
    setLayout(layout);
    resize(800, 600);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint |
                   Qt::WindowMinimizeButtonHint);

    turnLabel = new QLabel("Turn 0", this);
    turnLabel->setFont(QFont("Quicksand", 18, QFont::Bold));
    turnLabel->setStyleSheet(
        "QLabel { background-color: rgba(0, 0, 0, 180); "
        "color: white; padding: 8px 16px; "
        "font-size: 18px; font-weight: bold; "
        "border-radius: 4px; }");
    turnLabel->setMinimumWidth(120);
    turnLabel->setAlignment(Qt::AlignCenter);
    turnLabel->move(10, 10);
    turnLabel->raise();

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

    if (humanPlayer != nullptr)
        gameMap->bindMoveQueue(humanPlayer->getMoveQueue());

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

void LocalGameWindow::updateView(const BoardView& boardView) {
    int height = gameMap->mapHeight();
    int width = gameMap->mapWidth();
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            gameMap->tileAt(r, c) =
                toDisplayTile(boardView.tileAt(r + 1, c + 1));
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
    if (humanPlayer == nullptr || !game->isAlive(humanPlayer->playerId)) {
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
    gameMap->bindMoveQueue(nullptr);
}

void LocalGameWindow::updateLeaderboard(
    const std::vector<game::RankItem>& rank) {
    if (leaderboardWidget == nullptr || game == nullptr) {
        return;
    }

    std::vector<LeaderboardRow> rows;
    rows.reserve(rank.size());
    for (const auto& item : rank) {
        if (!game->isAlive(item.player)) {
            continue;
        }

        int totalLand = 0;
        for (int i = 0; i < TILE_TYPE_COUNT; ++i) {
            totalLand += item.land[i];
        }

        LeaderboardRow row;
        row.playerId = item.player;
        row.playerName = QString::fromStdString(game->getName(item.player));
        row.army = item.army;
        row.land = totalLand;
        row.playerColor = playerColor(item.player);
        rows.push_back(std::move(row));
    }

    leaderboardWidget->setRows(std::move(rows));
    positionFloatingWidgets();
}

void LocalGameWindow::positionFloatingWidgets() {
    const int margin = 6;

    if (leaderboardWidget != nullptr) {
        int x = width() - leaderboardWidget->width() - margin;
        if (x < margin) {
            x = margin;
        }
        leaderboardWidget->move(x, margin);
        leaderboardWidget->raise();
    }

    if (turnLabel != nullptr) {
        turnLabel->raise();
    }
}

void LocalGameWindow::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    positionFloatingWidgets();
}
