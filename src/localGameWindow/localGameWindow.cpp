#include "localGameWindow.h"

#include <QRandomGenerator>
#include <QTimer>
#include <QVBoxLayout>

void HumanPlayer::init(index_t playerId,
                       const game::GameConstantsPack& constants) {
    // TODO: HumanPlayer initialization
}

void HumanPlayer::requestMove(BoardView& boardView) {
    // TODO: send boardView to UI
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
    // testing configuration
    BoardView view =
        createRandomBoard(config.mapWidth, config.mapHeight).view(0);
    for (int r = 0; r < config.mapHeight; r++) {
        for (int c = 0; c < config.mapWidth; c++) {
            gameMap->tileAt(r, c) = toDisplayTile(view.tileAt(r + 1, c + 1));
        }
    }
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(gameMap);
    setLayout(layout);
    resize(800, 600);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint |
                   Qt::WindowMinimizeButtonHint);

    humanPlayer = new HumanPlayer();
    gameMap->bindMoveQueue(humanPlayer->getMoveQueue());

    QTimer::singleShot(0, [this]() { gameMap->fitCenter(25); });
}

LocalGameWindow::~LocalGameWindow() { delete humanPlayer; }
