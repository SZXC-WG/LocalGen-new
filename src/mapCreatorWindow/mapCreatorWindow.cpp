#include "mapCreatorWindow.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QPushButton>
#include <QVBoxLayout>

MapCreatorWindow::MapCreatorWindow(QWidget* parent)
    : QDialog(parent), selectedToolIndex(static_cast<int>(ToolType::MOUNTAIN)) {
    setWindowTitle("Map Creator");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);

    map = new MapWidget(this, 10, 10, false);
    connect(map, &MapWidget::cellClicked, this,
            &MapCreatorWindow::onMapClicked);

    setupToolbar();

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(map);

    setLayout(mainLayout);
    resize(800, 600);

    setFocusPolicy(Qt::StrongFocus);
}

MapCreatorWindow::~MapCreatorWindow() {}

void MapCreatorWindow::setupToolbar() {
    toolbar = new QWidget(this);
    toolbar->setFixedWidth(50);
    toolbar->setFixedHeight(9 * 40 + 8 * 3 + 20);
    toolbar->setStyleSheet(
        "QWidget { background-color: white; border-top-right-radius: 8px; "
        "border-bottom-right-radius: 8px; }");

    QVBoxLayout* toolLayout = new QVBoxLayout(toolbar);
    toolLayout->setContentsMargins(5, 5, 5, 5);
    toolLayout->setSpacing(3);

    QStringList toolIcons = {
        ":/images/svg/mountain.svg",    ":/images/svg/lookout.svg",
        ":/images/svg/observatory.svg", ":/images/svg/desert.svg",
        ":/images/svg/swamp.svg",       ":/images/svg/crown.svg",
        ":/images/svg/city.svg",        ":/images/svg/light.svg",
        ":/images/img/erase.png"};

    for (int i = 0; i < toolIcons.size(); ++i) {
        QPushButton* btn = new QPushButton(toolbar);
        btn->setFixedSize(40, 40);

        QIcon icon(toolIcons[i]);
        btn->setIcon(icon);
        btn->setIconSize(QSize(30, 30));
        btn->setProperty("toolIndex", i);
        btn->setStyleSheet(
            "QPushButton {"
            "    border: 2px solid transparent;"
            "    border-radius: 8px;"
            "    background-color: transparent;"
            "}"
            "QPushButton:hover {"
            "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
            "#f0f0f0, stop:1 #e0e0e0);"
            "    border: 2px solid #d0d0d0;"
            "}");

        connect(btn, &QPushButton::clicked, this,
                &MapCreatorWindow::onToolSelected);
        toolButtons.append(btn);
        toolLayout->addWidget(btn);
    }

    updateToolButtonStyles();
}

void MapCreatorWindow::onToolSelected() {
    QPushButton* sender = qobject_cast<QPushButton*>(this->sender());
    if (sender) {
        selectedToolIndex = sender->property("toolIndex").toInt();
        updateToolButtonStyles();
    }
}

void MapCreatorWindow::updateToolButtonStyles() {
    for (int i = 0; i < toolButtons.size(); ++i) {
        QPushButton* btn = toolButtons[i];
        if (i == selectedToolIndex) {
            btn->setStyleSheet(
                "QPushButton {"
                "    border: 2px solid #333333;"
                "    border-radius: 8px;"
                "    background-color: #f0f0f0;"
                "    color: black;"
                "}"
                "QPushButton:hover {"
                "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                "stop:0 #f0f0f0, stop:1 #e0e0e0);"
                "    border: 2px solid #333333;"
                "}");
        } else {
            btn->setStyleSheet(
                "QPushButton {"
                "    border: 2px solid transparent;"
                "    border-radius: 8px;"
                "    background-color: transparent;"
                "    opacity: 0.4;"
                "}"
                "QPushButton:hover {"
                "    background-color: #e8e8e8;"
                "    border: 2px solid transparent;"
                "    opacity: 0.6;"
                "}");
        }
    }
}

void MapCreatorWindow::onMapClicked(int r, int c) {
    auto& tile = map->tileAt(r, c);
    switch (static_cast<ToolType>(selectedToolIndex)) {
        case ToolType::MOUNTAIN:
            tile.type = TILE_MOUNTAIN, tile.color = QColor(187, 187, 187),
            tile.text.clear();
            break;
        case ToolType::LOOKOUT:
            tile.type = TILE_LOOKOUT, tile.color = QColor(187, 187, 187),
            tile.text.clear();
            break;
        case ToolType::OBSERVATORY:
            tile.type = TILE_OBSERVATORY, tile.color = QColor(187, 187, 187),
            tile.text.clear();
            break;
        case ToolType::DESERT:
            tile.type = TILE_DESERT, tile.color = QColor(220, 220, 220),
            tile.text.clear();
            break;
        case ToolType::SWAMP:
            tile.type = TILE_SWAMP, tile.color = QColor(128, 128, 128),
            tile.text.clear();
            break;
        case ToolType::CROWN:
            tile.type = TILE_GENERAL, tile.color = Qt::darkCyan,
            tile.text.clear();
            break;
        case ToolType::CITY:
            tile.type = TILE_CITY, tile.color = QColor(128, 128, 128),
            tile.text = "40";
            break;
        case ToolType::ERASE:
            tile.type = TILE_BLANK, tile.color = QColor(220, 220, 220),
            tile.lightIcon = false, tile.text.clear();
            break;
        case ToolType::LIGHT: tile.lightIcon = !tile.lightIcon;
    }
    map->repaint();
}

void MapCreatorWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Up:
            selectedToolIndex = (selectedToolIndex - 1 + toolButtons.size()) %
                                toolButtons.size();
            updateToolButtonStyles();
            break;
        case Qt::Key_Down:
            selectedToolIndex = (selectedToolIndex + 1) % toolButtons.size();
            updateToolButtonStyles();
            break;
        default: QDialog::keyPressEvent(event); break;
    }
}
