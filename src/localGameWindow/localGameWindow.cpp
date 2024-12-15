#include "localGameWindow.h"
#include <QVBoxLayout>

LocalGameWindow::LocalGameWindow(QWidget* parent) :
    QDialog(parent), gameMap(nullptr) {
    setWindowTitle("Local Game");
    gameMap = new MapWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(gameMap);
    setLayout(layout);
    resize(800, 600);
}

LocalGameWindow::~LocalGameWindow() {
}