#include "localGameWindow.h"

#include <QTimer>
#include <QVBoxLayout>

LocalGameWindow::LocalGameWindow(QWidget* parent, const LocalGameConfig& config)
    : QDialog(parent) {
    setWindowTitle("Local Game");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);
    gameMap = new MapWidget(this, config.mapWidth, config.mapHeight);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(gameMap);
    setLayout(layout);
    resize(800, 600);

    QTimer::singleShot(0, [this]() { gameMap->fitCenter(25); });
}

LocalGameWindow::~LocalGameWindow() {}
