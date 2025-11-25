#include "localGameWindow.h"

#include <QTimer>
#include <QVBoxLayout>

void HumanPlayer::init(index_t playerId,
                       const game::GameConstantsPack& constants) {
    // TODO: HumanPlayer initialization
}

void HumanPlayer::requestMove(BoardView& boardView) {
    // TODO: send boardView to UI
}

LocalGameWindow::LocalGameWindow(QWidget* parent, const LocalGameConfig& config)
    : QDialog(parent) {
    setWindowTitle("Local Game");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);

    gameMap = new MapWidget(this, config.mapWidth, config.mapHeight);
    // testing configuration
    for (int r = 10; r < config.mapHeight; r++) {
        for (int c = 10; c < config.mapWidth; c++) {
            gameMap->tileAt(r, c).color = QColor(39, 39, 39);
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
