#include "localGameWindow.h"
#include <QVBoxLayout>

LocalGameWindow::LocalGameWindow(QWidget* parent, const LocalGameConfig& config) :
    QDialog(parent) {
    setWindowTitle("Local Game");
    gameMap = new MapWidget(this, config.mapWidth, config.mapHeight);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(gameMap);
    setLayout(layout);
    resize(800, 600);
}

LocalGameWindow::~LocalGameWindow() {
}
