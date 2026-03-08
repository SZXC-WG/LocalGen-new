#include "mapCreatorWindow.h"

#include <QBitArray>
#include <QByteArray>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QResizeEvent>
#include <QSlider>
#include <QVBoxLayout>

#include "src/core/board.hpp"

#define MAGIC_6 quint32(0x4C47656E)

MapCreatorWindow::MapCreatorWindow(QWidget* parent)
    : QDialog(parent), networkManager(nullptr), selectedTool(MOUNTAIN) {
    setWindowTitle("Map Creator");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(36, 36, 36));
    setPalette(pal);

    map = new MapWidget(this, 10, 10, false, 100);
    connect(map, &MapWidget::cellClicked, this,
            &MapCreatorWindow::onMapClicked);

    setupToolbar();
    setupSliders();
    setupHintBar();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(map);

    setLayout(mainLayout);
    resize(800, 600);
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint |
                   Qt::WindowMinimizeButtonHint);
    repositionFloatingElements();

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
        ":/images/svg/city.svg",        ":/images/img/neutral.png",
        ":/images/svg/light.svg",       ":/images/img/erase.png"};

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
        selectedTool =
            static_cast<ToolType>(sender->property("toolIndex").toInt());
        updateToolButtonStyles();
        updateHintBar();
    }
}

void MapCreatorWindow::updateToolButtonStyles() {
    for (int i = 0; i < toolButtons.size(); ++i) {
        QPushButton* btn = toolButtons[i];
        if (i == selectedTool) {
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
    switch (selectedTool) {
        case MOUNTAIN:
            tile.type = TILE_MOUNTAIN, tile.color.setRgb(187, 187, 187),
            tile.text.clear();
            break;
        case LOOKOUT:
            tile.type = TILE_LOOKOUT, tile.color.setRgb(187, 187, 187),
            tile.text.clear();
            break;
        case OBSERVATORY:
            tile.type = TILE_OBSERVATORY, tile.color.setRgb(187, 187, 187),
            tile.text.clear();
            break;
        case DESERT:
            tile.type = TILE_DESERT, tile.color.setRgb(220, 220, 220),
            tile.text.clear();
            break;
        case SWAMP:
            tile.type = TILE_SWAMP, tile.color.setRgb(128, 128, 128),
            tile.text.clear();
            break;
        case SPAWN:
            tile.type = TILE_SPAWN, tile.color = Qt::darkCyan;
            tile.text = teamComboBox->currentText();
            break;
        case CITY:
            tile.type = TILE_CITY, tile.color.setRgb(128, 128, 128),
            tile.text = QString::number(valueSpinBox->value());
            break;
        case NEUTRAL:
            if (valueSpinBox->value() != 0) {
                tile.type = TILE_NEUTRAL, tile.color.setRgb(128, 128, 128),
                tile.text = QString::number(valueSpinBox->value());
                break;
            }  // neutral tiles of army strength 0 are equivalent to blank tiles
            [[fallthrough]];
        case ERASE:
            tile.type = TILE_BLANK, tile.color.setRgb(220, 220, 220),
            tile.lightIcon = false, tile.text.clear();
            break;
        case LIGHT: tile.lightIcon = !tile.lightIcon;
    }
    map->update();
}

void MapCreatorWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Up:
            selectedTool = static_cast<ToolType>(
                (selectedTool - 1 + toolButtons.size()) % toolButtons.size());
            updateToolButtonStyles();
            updateHintBar();
            break;
        case Qt::Key_Down:
            selectedTool =
                static_cast<ToolType>((selectedTool + 1) % toolButtons.size());
            updateToolButtonStyles();
            updateHintBar();
            break;
        case Qt::Key_C: map->fitCenter(); break;
        default:        QDialog::keyPressEvent(event); break;
    }
}

void MapCreatorWindow::setupSliders() {
    sliderContainer = new QWidget(this);
    sliderContainer->setFixedSize(550, 70);

    QHBoxLayout* containerLayout = new QHBoxLayout(sliderContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addStretch();

    QWidget* floatingPanel = new QWidget(sliderContainer);
    floatingPanel->setFixedSize(550, 70);
    floatingPanel->setStyleSheet(
        "QWidget { background-color: white; border-bottom-left-radius: 8px; "
        "border-bottom-right-radius: 8px; }");

    QVBoxLayout* panelLayout = new QVBoxLayout(floatingPanel);
    panelLayout->setContentsMargins(12, 6, 12, 6);
    panelLayout->setSpacing(8);

    QFont font("Quicksand", 10, QFont::Bold);
    QFont btnFont("Quicksand", 9, QFont::Bold);

    // Button row
    QHBoxLayout* buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(6);

    auto makeButton = [&](const QString& text, const QString& bg,
                          const QString& border, const QString& fg,
                          const QString& hoverBg, const QString& hoverBorder,
                          const QString& pressedBg) {
        QPushButton* btn = new QPushButton(text, floatingPanel);
        btn->setFont(btnFont);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        btn->setStyleSheet(
            QString("QPushButton {"
                    "    background-color: %1;"
                    "    border: 1px solid %2;"
                    "    border-radius: 4px;"
                    "    padding: 4px 12px;"
                    "    font-weight: bold;"
                    "    color: %3;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: %4;"
                    "    border: 1px solid %5;"
                    "}"
                    "QPushButton:pressed {"
                    "    background-color: %6;"
                    "}")
                .arg(bg, border, fg, hoverBg, hoverBorder, pressedBg));
        return btn;
    };

    QPushButton* openButton =
        makeButton("\xF0\x9F\x93\x82 Open", "#e8f5e9", "#a5d6a7", "#2e7d32",
                   "#c8e6c9", "#81c784", "#a5d6a7");
    QPushButton* saveButton =
        makeButton("\xF0\x9F\x92\xBE Save", "#fff3e0", "#ffcc80", "#e65100",
                   "#ffe0b2", "#ffb74d", "#ffcc80");
    QPushButton* importButton =
        makeButton("\xF0\x9F\x8C\x90 Import", "#e8f0fe", "#a8c7fa", "#1a73e8",
                   "#d2e3fc", "#7baaf7", "#aecbfa");

    connect(openButton, &QPushButton::clicked, this,
            &MapCreatorWindow::onOpenMap);
    connect(saveButton, &QPushButton::clicked, this,
            &MapCreatorWindow::onSaveMap);
    connect(importButton, &QPushButton::clicked, this,
            &MapCreatorWindow::onImportFromWeb);

    buttonRow->addWidget(openButton);
    buttonRow->addWidget(saveButton);
    buttonRow->addWidget(importButton);
    panelLayout->addLayout(buttonRow);

    // Slider row
    QHBoxLayout* sliderRow = new QHBoxLayout();
    sliderRow->setContentsMargins(0, 0, 0, 0);
    sliderRow->setSpacing(5);

    // Width slider
    QLabel* widthLabel = new QLabel("W:", floatingPanel);
    widthLabel->setFont(font);
    widthLabel->setStyleSheet("color: black;");
    widthSlider = new QSlider(Qt::Horizontal, floatingPanel);
    widthSlider->setRange(1, 100);
    widthSlider->setValue(10);
    QLabel* widthValueLabel = new QLabel("10", floatingPanel);
    widthValueLabel->setFont(font);
    widthValueLabel->setStyleSheet("color: black;");
    widthValueLabel->setAlignment(Qt::AlignCenter);
    widthValueLabel->setFixedWidth(30);

    connect(widthSlider, &QSlider::valueChanged,
            [this, widthValueLabel](int value) {
                widthValueLabel->setText(QString::number(value));
                map->setMapWidth(value);
            });

    sliderRow->addWidget(widthLabel);
    sliderRow->addWidget(widthSlider, 1);
    sliderRow->addWidget(widthValueLabel);

    sliderRow->addSpacing(10);

    // Height slider
    QLabel* heightLabel = new QLabel("H:", floatingPanel);
    heightLabel->setFont(font);
    heightLabel->setStyleSheet("color: black;");
    heightSlider = new QSlider(Qt::Horizontal, floatingPanel);
    heightSlider->setRange(1, 100);
    heightSlider->setValue(10);
    QLabel* heightValueLabel = new QLabel("10", floatingPanel);
    heightValueLabel->setFont(font);
    heightValueLabel->setStyleSheet("color: black;");
    heightValueLabel->setAlignment(Qt::AlignCenter);
    heightValueLabel->setFixedWidth(30);

    connect(heightSlider, &QSlider::valueChanged,
            [this, heightValueLabel](int value) {
                heightValueLabel->setText(QString::number(value));
                map->setMapHeight(value);
            });

    sliderRow->addWidget(heightLabel);
    sliderRow->addWidget(heightSlider, 1);
    sliderRow->addWidget(heightValueLabel);

    panelLayout->addLayout(sliderRow);

    containerLayout->addWidget(floatingPanel);
    containerLayout->addStretch();
}

void MapCreatorWindow::setupHintBar() {
    hintContainer = new QWidget(this);
    hintContainer->setFixedHeight(50);

    QHBoxLayout* containerLayout = new QHBoxLayout(hintContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addStretch();

    QWidget* floatingPanel = new QWidget(hintContainer);
    floatingPanel->setFixedHeight(50);
    floatingPanel->setStyleSheet(
        "QWidget { background-color: white; border-top-left-radius: 8px; "
        "border-top-right-radius: 8px; }");

    QHBoxLayout* hintLayout = new QHBoxLayout(floatingPanel);
    hintLayout->setContentsMargins(15, 10, 15, 10);
    hintLayout->setSpacing(10);

    QFont font("Quicksand", 10, QFont::Bold);

    hintLabel = new QLabel("Click a tile to place a mountain.", floatingPanel);
    hintLabel->setFont(font);
    hintLabel->setStyleSheet("color: black;");
    hintLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    valueLabel = new QLabel("City Strength:", floatingPanel);
    valueLabel->setFont(font);
    valueLabel->setStyleSheet("color: black;");
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    valueLabel->setVisible(false);

    valueSpinBox = new QSpinBox(floatingPanel);
    valueSpinBox->setRange(-9999, 9999);
    valueSpinBox->setValue(40);
    valueSpinBox->setMinimumWidth(80);
    valueSpinBox->setStyleSheet(
        "QSpinBox {"
        "    border: 1px solid #d0d0d0;"
        "    border-radius: 4px;"
        "    padding: 2px 5px;"
        "    background-color: white;"
        "    color: black;"
        "}"
        "QSpinBox:focus {"
        "    border: 2px solid #4CAF50;"
        "}");
    valueSpinBox->setVisible(false);

    teamComboBox = new QComboBox(floatingPanel);
    teamComboBox->addItem("");
    for (char c = 'A'; c <= 'Z'; ++c) {
        teamComboBox->addItem(QString(c));
    }
    teamComboBox->setMinimumWidth(30);
    teamComboBox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #d0d0d0;"
        "    border-radius: 4px;"
        "    padding: 2px 5px;"
        "    background-color: white;"
        "    color: black;"
        "}"
        "QComboBox:focus {"
        "    border: 2px solid #4CAF50;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "}"

        "QComboBox::down-arrow {"
        "    image: none;"
        "    border: none;"
        "}");
    teamComboBox->setVisible(false);

    hintLayout->addWidget(hintLabel);
    hintLayout->addWidget(valueLabel);
    hintLayout->addWidget(valueSpinBox);
    hintLayout->addWidget(teamComboBox);

    containerLayout->addWidget(floatingPanel);
    containerLayout->addStretch();

    updateHintBar();
}

void MapCreatorWindow::updateHintBar() {
    static const QStringList hints = {
        "Click a tile to place a mountain.",
        "Click a tile to place a Lookout.",
        "Click a tile to place an Observatory.",
        "Click a tile to place a desert. Deserts "
        "do not gain army bonus every 25 turns.",
        "Click a tile to place a swamp. Swamps drain 1 army per turn.",
        "Click a tile to place a spawn.",
        "Click a tile to place a city.",
        "Click a tile to place neutral army.",
        "Click a tile to toggle light tile. A light tile can be any tile, and "
        "is visible by every player, regardless if they are adjacent to it.",
        "Click a tile to remove it."};

    if (selectedTool == CITY || selectedTool == NEUTRAL) {
        hintLabel->setVisible(false);
        valueLabel->setVisible(true);
        valueSpinBox->setVisible(true);
        teamComboBox->setVisible(false);

        if (selectedTool == CITY) {
            valueLabel->setText("City Strength:");
        } else {
            valueLabel->setText("Neutral Army Strength:");
        }
    } else if (selectedTool == SPAWN) {
        hintLabel->setVisible(false);
        valueLabel->setVisible(true);
        valueSpinBox->setVisible(false);
        teamComboBox->setVisible(true);
        valueLabel->setText("General Team:");
    } else {
        hintLabel->setVisible(true);
        valueLabel->setVisible(false);
        valueSpinBox->setVisible(false);
        teamComboBox->setVisible(false);
        hintLabel->setText(hints[selectedTool]);
    }

    QWidget* floatingPanel = hintContainer->findChild<QWidget*>();
    if (floatingPanel) {
        floatingPanel->adjustSize();
        int panelWidth = floatingPanel->sizeHint().width();
        panelWidth = qMax(panelWidth, 100);
        floatingPanel->setFixedWidth(panelWidth);
        hintContainer->setFixedWidth(panelWidth);
        hintContainer->move((width() - hintContainer->width()) / 2,
                            height() - hintContainer->height());
        hintContainer->raise();
    }
}

void MapCreatorWindow::resizeEvent(QResizeEvent* event) {
    QDialog::resizeEvent(event);
    repositionFloatingElements();
}

void MapCreatorWindow::repositionFloatingElements() {
    if (sliderContainer && toolbar && hintContainer) {
        sliderContainer->move((width() - sliderContainer->width()) / 2, 0);
        sliderContainer->raise();

        toolbar->move(0, (height() - toolbar->height()) / 2);
        toolbar->raise();

        hintContainer->move((width() - hintContainer->width()) / 2,
                            height() - hintContainer->height());
        hintContainer->raise();
    }
}

const QString MapCreatorWindow::mapFileFilter =
    "All Supported Maps (*.lg *.lgmp *.json);;"
    "Official Generals.io Map (*.json);;"
    "LocalGen v5 (*.lg) - Legacy Format;;"
    "LocalGen v6 (*.lgmp) - Current Format;;";

void MapCreatorWindow::onOpenMap() {
    QString filename =
        QFileDialog::getOpenFileName(this, "Open Map", "", mapFileFilter);
    if (filename.isEmpty()) return;

    if (filename.endsWith(".lg"))
        openMap_v5(filename);
    else if (filename.endsWith(".lgmp"))
        openMap_v6(filename);
    else if (filename.endsWith(".json")) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Error", "Failed to open map file.");
            return;
        }
        QByteArray data = file.readAll();
        file.close();
        openOfficialMap(data);
    } else {
        QMessageBox::critical(this, "Error",
                              "Unsupported file format. Please select a .lg, "
                              ".lgmp, or .json file.");
        return;
    }

    map->fitCenter();
    widthSlider->setValue(map->mapWidth());
    heightSlider->setValue(map->mapHeight());
}

void MapCreatorWindow::openMap_v5(const QString& filename) {
    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to open map file.");
        return;
    }
    QString mapData = mapFile.readLine();
    mapFile.close();
    InitBoard board;
    board.v5Unzip(mapData.toStdString());
    int width = board.getWidth(), height = board.getHeight();
    map->realloc(width, height);
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            auto& tile = map->tileAt(r, c);
            const Tile& loadedTile = board.tileAt({r + 1, c + 1});
            tile.type = loadedTile.type;
            switch (loadedTile.type) {
                case TILE_MOUNTAIN:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY: tile.color.setRgb(187, 187, 187); break;
                case TILE_DESERT:      tile.color.setRgb(220, 220, 220); break;
                case TILE_SWAMP:
                case TILE_CITY:        tile.color.setRgb(128, 128, 128); break;
                case TILE_SPAWN:       tile.color = Qt::darkCyan; break;
                default:
                    tile.color = loadedTile.army == 0 ? QColor(220, 220, 220)
                                                      : QColor(128, 128, 128);
            };
            if (loadedTile.army != 0 || loadedTile.type == TILE_CITY)
                tile.text = QString::number(loadedTile.army);
            else
                tile.text.clear();
            tile.lightIcon = loadedTile.lit;
        }
    }
}

void MapCreatorWindow::openMap_v6(const QString& filename) {
    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open map file.");
        return;
    }

    QDataStream ds(&mapFile);
    ds.setVersion(QDataStream::Qt_6_7);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = 0;
    ds >> magic;
    if (magic != MAGIC_6) {
        QMessageBox::critical(this, "Error",
                              "Invalid map file: bad magic number.");
        return;
    }

    quint16 w16, h16;
    QByteArray compressed;
    ds >> w16 >> h16 >> compressed;

    if (ds.status() != QDataStream::Ok) {
        QMessageBox::critical(this, "Error",
                              "Invalid or corrupted map file format.");
        return;
    }

    // decompress map data
    const int width = w16, height = h16;
    QByteArray raw = qUncompress(compressed);
    if (raw.isEmpty()) {
        QMessageBox::critical(
            this, "Error",
            "Failed to decompress map data. The file might be corrupted.");
        return;
    }

    const int expectedBits = width * height * 19;
    const int expectedBytes = (expectedBits + 7) / 8;
    if (raw.size() != expectedBytes) {
        QMessageBox::critical(
            this, "Error",
            "Decompressed data size mismatch. The file is corrupted.");
        return;
    }

    QBitArray bits = QBitArray::fromBits(raw.constData(), expectedBits);

    // unpack tile from 19 bits
    auto unpackTile = [](quint32 p) -> DisplayTile {
        DisplayTile tile;
        tile.type = static_cast<tile_type_e>((p >> 16) & 0x7);
        tile.lightIcon = ((p >> 15) & 0x1);
        int val = (p & 0x7FFF) - 16384;
        tile.text = tile.type == TILE_CITY ? QString::number(val)
                    : val == 0             ? QString()
                    : tile.type == TILE_SPAWN
                        ? QString(QLatin1Char('A' + val - 1))
                        : QString::number(val);
        switch (tile.type) {
            case TILE_MOUNTAIN:
            case TILE_LOOKOUT:
            case TILE_OBSERVATORY: tile.color.setRgb(187, 187, 187); break;
            case TILE_DESERT:      tile.color.setRgb(220, 220, 220); break;
            case TILE_SWAMP:
            case TILE_CITY:        tile.color.setRgb(128, 128, 128); break;
            case TILE_SPAWN:       tile.color = Qt::darkCyan; break;
            default:
                tile.color = tile.text.isEmpty() ? QColor(220, 220, 220)
                                                 : QColor(128, 128, 128);
        };
        return tile;
    };

    map->realloc(width, height);
    int offset = 0;
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            quint32 packed = 0;
            for (int b = 0; b < 19; ++b, ++offset)
                if (bits.testBit(offset)) packed |= (1u << b);
            map->tileAt(r, c) = unpackTile(packed);
        }
    }
}

void MapCreatorWindow::openOfficialMap(const QByteArray& data) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::critical(this, "Error", "This is not a valid JSON file.");
        return;
    }

    QJsonObject obj = doc.object();
    int width = obj.value(QLatin1StringView("width")).toInt(),
        height = obj.value(QLatin1StringView("height")).toInt();
    QString mapStr = obj.value(QLatin1StringView("map")).toString();
    if (width <= 0 || height <= 0 || mapStr.isEmpty()) {
        QMessageBox::critical(this, "Error",
                              "Missing or invalid map configuration.");
        return;
    }

    QStringList tileList = mapStr.split(',');
    if (tileList.size() != width * height) {
        QMessageBox::critical(this, "Error",
                              QString("Inconsistent map data: number of tiles "
                                      "(%1) does not match width*height (%2).")
                                  .arg(tileList.size())
                                  .arg(width * height));
        return;
    }

    bool hasBadTiles = false;
    map->realloc(width, height);
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            QString tileCode = tileList[r * width + c].trimmed();
            auto& tile = map->tileAt(r, c);
            tile.text.clear();
            if (tileCode.startsWith("L_")) {
                tile.lightIcon = true;
                tileCode = tileCode.mid(2);
            }
            if (tileCode.isEmpty()) {
                tile.type = TILE_BLANK;
                tile.color.setRgb(220, 220, 220);
                continue;
            }
            bool ok;
            int army = tileCode.toInt(&ok);
            if (ok) {
                tile.text = QString::number(army);
                tile.type = TILE_CITY;
                tile.color.setRgb(128, 128, 128);
                continue;
            }
            switch (tileCode.at(0).unicode()) {
                case 'g':
                    tile.type = TILE_SPAWN;
                    tile.color = Qt::darkCyan;
                    tile.text = tileCode.mid(1);
                    continue;
                case 'm':
                    tile.type = TILE_MOUNTAIN;
                    tile.color.setRgb(187, 187, 187);
                    continue;
                case 'l':
                    tile.type = TILE_LOOKOUT;
                    tile.color.setRgb(187, 187, 187);
                    continue;
                case 'o':
                    tile.type = TILE_OBSERVATORY;
                    tile.color.setRgb(187, 187, 187);
                    continue;
                case 'd':
                    tile.type = TILE_DESERT;
                    tile.color.setRgb(220, 220, 220);
                    continue;
                case 's':
                    tile.type = TILE_SWAMP;
                    tile.color.setRgb(128, 128, 128);
                    continue;
                case 'n':
                    tile.type = TILE_NEUTRAL;
                    tile.color.setRgb(128, 128, 128);
                    tile.text = tileCode.mid(1);
                    continue;
                default:
                    tile.type = TILE_BLANK;
                    tile.color = Qt::red;
                    tile.text = "ERR";
                    hasBadTiles = true;
            }
        }
    }
    if (hasBadTiles) {
        QMessageBox::warning(this, "Warning",
                             "Some tiles are not recognized. "
                             "Please edit or truncate them before saving.");
    }
}

void MapCreatorWindow::onImportFromWeb() {
    bool ok;
    QString mapName = QInputDialog::getText(
        this, "Import from Generals.io",
        "Enter the map name:", QLineEdit::Normal, "", &ok);
    if (!ok || mapName.trimmed().isEmpty()) return;

    auto urlName = QUrl::toPercentEncoding(mapName.trimmed());
    QNetworkRequest request(
        QUrl("https://generals.io/api/map?name=" + urlName));
    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        "LocalGen/1.0 (+https://github.com/SZXC-WG/LocalGen-new)");

    if (!networkManager) {
        networkManager = new QNetworkAccessManager(this);
    }

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(
                this, "Network Error",
                "Failed to fetch map: " + reply->errorString());
            return;
        }
        QByteArray data = reply->readAll();
        openOfficialMap(data);
        map->fitCenter();
        widthSlider->setValue(map->mapWidth());
        heightSlider->setValue(map->mapHeight());
    });
}

void MapCreatorWindow::onSaveMap() {
    QString filename =
        QFileDialog::getSaveFileName(this, "Save Map", "", mapFileFilter);
    if (filename.isEmpty()) return;

    if (filename.endsWith(".lg"))
        saveMap_v5(filename);
    else if (filename.endsWith(".lgmp"))
        saveMap_v6(filename);
    else
        QMessageBox::critical(this, "Error",
                              "Unsupported file format. Please select a .lg or "
                              ".lgmp file.");
}

void MapCreatorWindow::saveMap_v5(const QString& filename) {
    int width = map->mapWidth(), height = map->mapHeight();

    // v5 map format
    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to open map file.");
        return;
    }

    InitBoard board(height, width);
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            const auto& tile = map->tileAt(r, c);
            Tile boardTile;
            boardTile.type = tile.type;
            boardTile.lit = tile.lightIcon;
            // v5 encoding does not support general teams,
            // so this field is ignored for now
            // TODO: add warning if team is set
            boardTile.army = tile.text.isEmpty() || tile.type == TILE_SPAWN
                                 ? 0
                                 : tile.text.toInt();
            board.changeTile({r + 1, c + 1}, boardTile);
        }
    }

    mapFile.write(QString::fromStdString(board.v5Zip()).toUtf8());
}

void MapCreatorWindow::saveMap_v6(const QString& filename) {
    int width = map->mapWidth(), height = map->mapHeight();

    // 19 bits per tile, thus packed into uint32
    auto packTile = [](const DisplayTile& tile) -> quint32 {
        // [18..16] type (3) | [15] lit | [14..0] army+16384
        int type = tile.type, lit = tile.lightIcon,
            val = tile.text.isEmpty() ? 0
                  : tile.type == TILE_SPAWN
                      ? tile.text.at(0).unicode() - 'A' + 1
                      : tile.text.toInt();
        return (type << 16) | (lit << 15) | (val + 16384);
    };

    QBitArray bits(height * width * 19);
    int offset = 0;
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            quint32 packed = packTile(map->tileAt(r, c));
            for (int b = 0; b < 19; ++b, ++offset) {
                bits.setBit(offset, (packed >> b) & 1);
            }
        }
    }

    // deflate
    const int byteLen = (bits.size() + 7) / 8;
    const uchar* src = reinterpret_cast<const uchar*>(bits.bits());
    QByteArray compressed = qCompress(src, byteLen, 9);

    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open map file.");
        return;
    }

    QDataStream ds(&mapFile);
    ds.setVersion(QDataStream::Qt_6_7);
    ds.setByteOrder(QDataStream::LittleEndian);

    ds << MAGIC_6 << quint16(width) << quint16(height) << compressed;

    mapFile.close();
}
