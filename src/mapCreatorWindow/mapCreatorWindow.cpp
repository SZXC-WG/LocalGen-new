#include "mapCreatorWindow.h"

#include <QVBoxLayout>

MapCreatorWindow::MapCreatorWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Map Creator");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);
    map = new MapWidget(this, 10, 10, false);
    connect(map, &MapWidget::cellClicked, this,
            &MapCreatorWindow::onMapClicked);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(map);
    setLayout(layout);
    resize(800, 600);
}

void MapCreatorWindow::onMapClicked(int r, int c) {
    map->tileAt(r, c).type = TILE_CITY;
    map->repaint();
}

MapCreatorWindow::~MapCreatorWindow() {}
