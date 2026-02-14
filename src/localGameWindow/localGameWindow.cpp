#include "localGameWindow.h"

#include <QElapsedTimer>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

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

    gameMap = new MapWidget(this, config.mapWidth, config.mapHeight);
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

    for (const auto& playerConfig : config.players) {
        if (playerConfig.name == "Human") {
            humanPlayer = new HumanPlayer();
            humanPlayer->setBoardViewHandler(
                [this](const BoardView& boardView) { updateView(boardView); });
            players.push_back(humanPlayer);
            teams.push_back(static_cast<index_t>(teams.size()));
            names.push_back("Human");
            continue;
        }
        BasicBot* bot =
            BotFactory::instance().create(playerConfig.name.toStdString());
        if (bot == nullptr) {
            continue;
        }
        players.push_back(bot);
        teams.push_back(static_cast<index_t>(teams.size()));
        names.push_back(playerConfig.name.toStdString());
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
    turnLabel->setFont(parent->font());
    turnLabel->setStyleSheet(
        "QLabel { background-color: rgba(0, 0, 0, 180); "
        "color: white; padding: 8px 16px; "
        "font-size: 18px; font-weight: bold; "
        "border-radius: 4px; }");
    turnLabel->setMinimumWidth(120);
    turnLabel->setAlignment(Qt::AlignCenter);
    turnLabel->move(10, 10);
    turnLabel->raise();

    if (humanPlayer != nullptr)
        gameMap->bindMoveQueue(humanPlayer->getMoveQueue());

    QTimer::singleShot(0, [this]() { gameMap->fitCenter(25); });
    if (game != nullptr) {
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
