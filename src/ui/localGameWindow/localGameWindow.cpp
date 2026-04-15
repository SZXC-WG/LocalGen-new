// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localGameWindow.h"

#include <QElapsedTimer>
#include <QFont>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

#include "core/bot.h"
#include "core/game.hpp"
#include "core/map.hpp"

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
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint |
                   Qt::WindowMinimizeButtonHint);

    setWindowTitle("Local Game");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);

    Board initialBoard;
    if (config.mapFilePath.isEmpty()) {
        std::mt19937::result_type seed = QRandomGenerator::global()->generate();
        initialBoard = Board::generate(config.mapWidth, config.mapHeight,
                                       config.players.size(), seed);
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
    gameMap->installEventFilter(this);
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

    for (const QString& name : config.players) {
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
    turnLabel->setFont(QFont("Quicksand", 12, QFont::Bold));
    turnLabel->setAutoFillBackground(true);
    QPalette palette = turnLabel->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::WindowText, Qt::black);
    turnLabel->setPalette(palette);
    turnLabel->setContentsMargins(16, 8, 16, 8);
    turnLabel->setMinimumWidth(120);
    turnLabel->setAlignment(Qt::AlignCenter);
    turnLabel->move(10, 10);
    turnLabel->raise();

    chatBox = new ChatBoxWidget(this);
    connect(chatBox, &ChatBoxWidget::messageSubmitted, this,
            &LocalGameWindow::handleChatSubmission);

    if (analysisEnabled) {
        std::vector<QColor> colors;
        std::vector<QString> playerNames;
        for (index_t i = 0; i < game->getPlayerCount(); ++i) {
            colors.push_back(playerColor(i));
            playerNames.push_back(QString::fromStdString(game->getName(i)));
        }
        analysisChartWidget = new AnalysisChartWidget(
            this, game->getPlayerCount(), colors, playerNames);
    }

    leaderboardWidget = new LeaderboardWidget(this);
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

    game->setEventCallback(
        [this](const GameEvent& event) { handleGameEvent(event); });
    updateLeaderboard(game->ranklist());
    gameRunning = true;
    refreshChatInputState();
    positionFloatingWidgets();
    scheduleNextHalfTurn(0.0);
}

LocalGameWindow::~LocalGameWindow() {
    stopGameLoop();
    if (game != nullptr) {
        game->setEventCallback({});
        delete game;
        game = nullptr;
        humanPlayer = nullptr;
    }
}

bool LocalGameWindow::canHumanChat() const {
    return game != nullptr && humanPlayerId >= 0;
}

void LocalGameWindow::handleChatSubmission(const QString& message) {
    if (canHumanChat()) humanPlayer->sendMessage(message.toStdString());
}

void LocalGameWindow::handleGameEvent(const GameEvent& event) {
    chatBox->appendMessage(formatChatMessage(event));
}

ChatMessageEntry LocalGameWindow::formatChatMessage(
    const GameEvent& event) const {
    ChatMessageEntry entry;
    entry.turnText = QString::number(event.turn);
    if (event.halfTurnPhase != 0) {
        entry.turnText.append('.');
    }

    auto appendText = [&entry](const QString& text) {
        if (!text.isEmpty()) {
            entry.segments.push_back(ChatMessageSegment{text, false, {}});
        }
    };
    auto appendPlayer = [this, &entry](index_t playerId,
                                       const QString& suffix = QString()) {
        entry.segments.push_back(ChatMessageSegment{
            playerDisplayName(playerId) + suffix, true, playerColor(playerId)});
    };

    std::visit(overloaded{[&](const GameMessageWin& msg) {
                              appendPlayer(msg.winner);
                              appendText("wins!");
                          },
                          [&](const GameMessageCapture& msg) {
                              appendPlayer(msg.capturer);
                              appendText("captured");
                              appendPlayer(msg.captured, ".");
                          },
                          [&](const GameMessageSurrender& msg) {
                              appendPlayer(msg.player);
                              appendText("surrendered.");
                          },
                          [&](const GameMessageText& msg) {
                              if (msg.sender != -1)
                                  appendPlayer(msg.sender, ":");
                              appendText(QString::fromStdString(msg.text));
                          }},
               event.data);
    return entry;
}

QString LocalGameWindow::playerDisplayName(index_t playerId) const {
    if (game != nullptr && playerId >= 0 && playerId < game->getPlayerCount()) {
        return QString::fromStdString(game->getName(playerId));
    }
    return QString("Player %1").arg(playerId);
}

void LocalGameWindow::refreshChatInputState() {
    QString placeholder;
    if (game == nullptr) {
        placeholder = "Chat is unavailable.";
    } else if (humanPlayerId < 0) {
        placeholder = "Only a human player can chat.";
    } else {
        placeholder = "Press [Enter] to chat.";
    }

    chatBox->setInputEnabled(canHumanChat(), placeholder);
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
        gameMap->bindMoveQueue(nullptr);
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
    if (!gameRunning) return;
    int waitMs = static_cast<int>(std::lround(delayMs));
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

    if (analysisChartWidget != nullptr) {
        analysisChartWidget->updateAnalysis(rows);
    }
    leaderboardWidget->setRows(std::move(rows));
    positionFloatingWidgets();
}

void LocalGameWindow::positionFloatingWidgets() {
    const int turnLabelMargin = 6;

    if (turnLabel != nullptr) {
        turnLabel->move(turnLabelMargin, turnLabelMargin);
        turnLabel->raise();
    }

    if (analysisChartWidget != nullptr) {
        const int analysisWidth = std::clamp(width() / 3, 340, 560);
        const int analysisHeight = std::clamp(height() / 2, 340, 520);
        const int x = std::max(0, width() - analysisWidth);
        const int y = std::max(0, height() - analysisHeight);
        analysisChartWidget->setGeometry(x, y, analysisWidth, analysisHeight);
        analysisChartWidget->raise();
    }

    if (leaderboardWidget != nullptr) {
        const int x = std::max(0, width() - leaderboardWidget->width());
        leaderboardWidget->move(x, 0);
        leaderboardWidget->raise();
    }

    if (chatBox != nullptr) {
        int rightBoundary = width();
        if (analysisChartWidget != nullptr) {
            rightBoundary = std::min(rightBoundary, analysisChartWidget->x());
        }
        if (leaderboardWidget != nullptr) {
            rightBoundary = std::min(rightBoundary, leaderboardWidget->x());
        }

        const int x = 0;
        const int availableWidth = std::max(0, rightBoundary - x);
        const int desiredWidth = std::clamp(width() / 4, 280, 380);
        const int panelWidth = std::min(desiredWidth, availableWidth);
        const int availableHeight = std::max(0, height());
        const int desiredHeight = std::clamp(height() / 3, 220, 340);
        const int panelHeight = std::min(desiredHeight, availableHeight);
        const int y = std::max(0, height() - panelHeight);

        if (panelWidth > 0 && panelHeight > 0) {
            chatBox->setGeometry(x, y, panelWidth, panelHeight);
            chatBox->show();
            chatBox->raise();
        } else {
            chatBox->hide();
        }
    }

    if (turnLabel != nullptr) turnLabel->raise();
}

bool LocalGameWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == gameMap && event->type() == QEvent::KeyPress &&
        canHumanChat() && chatBox != nullptr) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return ||
            keyEvent->key() == Qt::Key_Enter) {
            chatBox->focusInput();
            return true;
        }
    }

    return QDialog::eventFilter(watched, event);
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
