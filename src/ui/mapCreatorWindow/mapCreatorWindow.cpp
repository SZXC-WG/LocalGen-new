#include "mapCreatorWindow.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QEasingCurve>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QResizeEvent>
#include <QSlider>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/board.hpp"
#include "core/map.hpp"

namespace {
constexpr int METADATA_SIDEBAR_EXPANDED_WIDTH = 236;
constexpr int METADATA_SIDEBAR_COLLAPSED_WIDTH = 42;
constexpr int METADATA_SIDEBAR_COLLAPSED_HEIGHT = 156;
constexpr int METADATA_SIDEBAR_ANIMATION_MS = 120;
}  // namespace

MapCreatorWindow::MapCreatorWindow(QWidget* parent)
    : QDialog(parent),
      networkManager(nullptr),
      metadataSidebarExpanded(true),
      selectedTool(MOUNTAIN) {
    // Treat the editor like an independent top-level window rather than a
    // transient dialog attached to the main menu.
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::Window;
    flags |= Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint;
    flags &= ~Qt::Dialog;
    setWindowFlags(flags);

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
    setupMetadataSidebar();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(map);

    setLayout(mainLayout);
    resize(800, 600);
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

void MapCreatorWindow::setupMetadataSidebar() {
    sidebarContainer = new QWidget(this);
    sidebarContainer->setObjectName("metadataSidebar");
    sidebarContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sidebarContainer->setAttribute(Qt::WA_StyledBackground, true);
    sidebarContainer->setStyleSheet(
        "QWidget#metadataSidebar { background: transparent; }");

    sidebarPanel = new QWidget(sidebarContainer);
    sidebarPanel->setObjectName("metadataSidebarPanel");
    sidebarPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebarPanel);
    sidebarLayout->setContentsMargins(16, 32, 16, 32);
    sidebarLayout->setSpacing(18);

    sidebarHeader = new QWidget(sidebarPanel);
    QHBoxLayout* headerLayout = new QHBoxLayout(sidebarHeader);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    sidebarTitleLabel = new QLabel("Map Metadata", sidebarHeader);
    sidebarTitleLabel->setObjectName("sidebarTitleLabel");
    sidebarTitleLabel->setFont(QFont("Quicksand", 13, QFont::Bold));

    headerLayout->addWidget(sidebarTitleLabel);
    headerLayout->addStretch();

    sidebarToggleButton = new QToolButton(sidebarContainer);
    sidebarToggleButton->setFixedSize(METADATA_SIDEBAR_COLLAPSED_WIDTH,
                                      METADATA_SIDEBAR_COLLAPSED_HEIGHT);
    sidebarToggleButton->setFont(QFont("Segoe UI Symbol", 24));
    sidebarToggleButton->setAttribute(Qt::WA_Hover, false);
    sidebarToggleButton->setStyleSheet(
        "QToolButton {"
        "    border: none;"
        "    border-top-left-radius: 10px;"
        "    border-bottom-left-radius: 10px;"
        "    background-color: rgba(8, 8, 8, 179);"
        "    color: white;"
        "    padding: 0;"
        "}");

    metadataFormContainer = new QWidget(sidebarContainer);
    QFormLayout* formLayout = new QFormLayout(metadataFormContainer);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setRowWrapPolicy(QFormLayout::WrapAllRows);
    formLayout->setHorizontalSpacing(0);
    formLayout->setVerticalSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mapTitleEdit = new QLineEdit(metadataFormContainer);
    mapTitleEdit->setPlaceholderText("Enter map title");
    mapTitleEdit->setClearButtonEnabled(true);

    authorEdit = new QLineEdit(metadataFormContainer);
    authorEdit->setPlaceholderText("Enter author name");
    authorEdit->setClearButtonEnabled(true);

    creationDateEdit =
        new QDateTimeEdit(QDateTime::currentDateTime(), metadataFormContainer);
    creationDateEdit->setCalendarPopup(true);
    creationDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

    descriptionEdit = new QTextEdit(metadataFormContainer);
    descriptionEdit->setPlaceholderText("Enter map description");
    descriptionEdit->setAcceptRichText(false);
    descriptionEdit->setTabChangesFocus(true);
    descriptionEdit->setMinimumHeight(96);

    formLayout->addRow("Title", mapTitleEdit);
    formLayout->addRow("Author", authorEdit);
    formLayout->addRow("Created", creationDateEdit);
    formLayout->addRow("Description", descriptionEdit);

    sidebarLayout->addStretch();
    sidebarLayout->addWidget(sidebarHeader);
    sidebarLayout->addWidget(metadataFormContainer);
    sidebarLayout->addStretch();

    sidebarPanel->setStyleSheet(
        "QWidget#metadataSidebarPanel {"
        "    background-color: rgba(8, 8, 8, 179);"
        "}"
        "QLabel { color: white; }"
        "QLabel#sidebarTitleLabel { color: rgba(255, 255, 255, 0.98); }"
        "QLineEdit, QDateTimeEdit, QTextEdit {"
        "    min-height: 34px;"
        "    border: 1px solid rgba(255, 255, 255, 0.16);"
        "    border-radius: 6px;"
        "    padding: 4px 10px;"
        "    background-color: rgba(255, 255, 255, 0.08);"
        "    color: white;"
        "}"
        "QLineEdit::placeholder { color: rgba(255, 255, 255, 0.55); }"
        "QTextEdit { padding-top: 8px; padding-bottom: 8px; }"
        "QLineEdit:focus, QDateTimeEdit:focus, QTextEdit:focus {"
        "    border: 1px solid rgba(255, 255, 255, 0.38);"
        "    background-color: rgba(255, 255, 255, 0.12);"
        "}"
        "QDateTimeEdit::drop-down {"
        "    subcontrol-origin: padding;"
        "    width: 24px;"
        "    border: none;"
        "}"
        "QToolButton { border: none; background: transparent; }");

    connect(sidebarToggleButton, &QToolButton::clicked, this,
            &MapCreatorWindow::toggleMetadataSidebar);

    sidebarAnimation =
        new QPropertyAnimation(sidebarContainer, "geometry", this);
    sidebarAnimation->setDuration(METADATA_SIDEBAR_ANIMATION_MS);
    sidebarAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(sidebarAnimation, &QPropertyAnimation::finished, this,
            [this]() { applyMetadataSidebarState(); });

    resetMapMetadata();
    updateMetadataSidebar();
}

void MapCreatorWindow::toggleMetadataSidebar() {
    metadataSidebarExpanded = !metadataSidebarExpanded;
    updateMetadataSidebar(true);
}

void MapCreatorWindow::updateMetadataSidebar(bool animate) {
    if (!sidebarContainer) return;

    if (sidebarAnimation &&
        sidebarAnimation->state() == QAbstractAnimation::Running) {
        sidebarAnimation->stop();
    }

    if (!animate) {
        applyMetadataSidebarState();
        sidebarContainer->setGeometry(
            metadataSidebarGeometry(metadataSidebarExpanded));
        sidebarContainer->raise();
        return;
    }

    const QRect expandedGeometry = metadataSidebarGeometry(true);
    const QRect hiddenGeometry = metadataSidebarGeometry(false);

    sidebarContainer->setMinimumSize(0, 0);
    sidebarContainer->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    applyMetadataSidebarState();
    sidebarAnimation->setStartValue(sidebarContainer->geometry());
    sidebarAnimation->setEndValue(metadataSidebarExpanded ? expandedGeometry
                                                          : hiddenGeometry);

    sidebarContainer->raise();
    sidebarAnimation->start();
}

void MapCreatorWindow::applyMetadataSidebarState() {
    if (!sidebarContainer || !sidebarPanel) return;

    QVBoxLayout* sidebarLayout =
        qobject_cast<QVBoxLayout*>(sidebarPanel->layout());

    sidebarToggleButton->setText(
        QString::fromUtf8(metadataSidebarExpanded ? "❯" : "❮"));
    if (sidebarLayout) {
        sidebarLayout->setContentsMargins(16, 32, 16, 32);
        sidebarLayout->setSpacing(18);
    }

    sidebarContainer->setFixedWidth(METADATA_SIDEBAR_EXPANDED_WIDTH +
                                    METADATA_SIDEBAR_COLLAPSED_WIDTH);
    sidebarContainer->setFixedHeight(height());
    sidebarPanel->setGeometry(METADATA_SIDEBAR_COLLAPSED_WIDTH, 0,
                              METADATA_SIDEBAR_EXPANDED_WIDTH, height());
    sidebarHeader->setVisible(true);
    metadataFormContainer->setVisible(true);
    sidebarPanel->show();
    sidebarContainer->show();

    sidebarToggleButton->setGeometry(metadataSidebarButtonGeometry());
    sidebarToggleButton->raise();
}

QRect MapCreatorWindow::metadataSidebarGeometry(bool expanded) const {
    if (!map) return QRect();

    const int sidebarWidth =
        METADATA_SIDEBAR_EXPANDED_WIDTH + METADATA_SIDEBAR_COLLAPSED_WIDTH;
    const int sidebarHeight = height();
    const int sidebarY = 0;
    const int sidebarX = expanded ? width() - sidebarWidth
                                  : width() - METADATA_SIDEBAR_COLLAPSED_WIDTH;
    return QRect(sidebarX, sidebarY, sidebarWidth, sidebarHeight);
}

QRect MapCreatorWindow::metadataSidebarButtonGeometry() const {
    return QRect(
        0, (sidebarContainer->height() - METADATA_SIDEBAR_COLLAPSED_HEIGHT) / 2,
        METADATA_SIDEBAR_COLLAPSED_WIDTH, METADATA_SIDEBAR_COLLAPSED_HEIGHT);
}

void MapCreatorWindow::setMapMetadata(const MapMetadata& metadata) {
    mapTitleEdit->setText(metadata.title);
    authorEdit->setText(metadata.author);
    creationDateEdit->setDateTime(metadata.creationDateTime.isValid()
                                      ? metadata.creationDateTime
                                      : QDateTime::currentDateTime());
    descriptionEdit->setPlainText(metadata.description);
}

MapMetadata MapCreatorWindow::currentMetadata() const {
    return {mapTitleEdit->text(), authorEdit->text(),
            creationDateEdit->dateTime().isValid()
                ? creationDateEdit->dateTime()
                : QDateTime::currentDateTime(),
            descriptionEdit->toPlainText().trimmed()};
}

void MapCreatorWindow::resetMapMetadata(const QString& mapTitle,
                                        const QString& author,
                                        const QDateTime& creationDateTime,
                                        const QString& description) {
    setMapMetadata({mapTitle, author,
                    creationDateTime.isValid() ? creationDateTime
                                               : QDateTime::currentDateTime(),
                    description});
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
    teamComboBox->setMaxVisibleItems(12);
    teamComboBox->view()->setVerticalScrollMode(
        QAbstractItemView::ScrollPerPixel);
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
    updateMetadataSidebar();
    repositionFloatingElements();
}

void MapCreatorWindow::repositionFloatingElements() {
    if (sliderContainer && toolbar && hintContainer && sidebarContainer &&
        map) {
        const QRect canvasRect = map->geometry();
        const int canvasTop = canvasRect.top(),
                  canvasBottom = canvasRect.bottom(),
                  canvasLeft = canvasRect.left(),
                  canvasWidth = canvasRect.width(),
                  canvasHeight = canvasRect.height();

        sliderContainer->move(
            canvasLeft + (canvasWidth - sliderContainer->width()) / 2, 0);
        sliderContainer->raise();

        toolbar->move(canvasLeft,
                      canvasTop + (canvasHeight - toolbar->height()) / 2);
        toolbar->raise();

        hintContainer->move(
            canvasLeft + (canvasWidth - hintContainer->width()) / 2,
            canvasBottom - hintContainer->height() + 1);
        hintContainer->raise();

        if (!sidebarAnimation ||
            sidebarAnimation->state() != QAbstractAnimation::Running) {
            sidebarContainer->setGeometry(
                metadataSidebarGeometry(metadataSidebarExpanded));
        }
        sidebarContainer->raise();
    }
}

Board MapCreatorWindow::toBoard() const {
    const int width = map->mapWidth(), height = map->mapHeight();
    Board board(height, width);
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            const DisplayTile& tile = map->tileAt(r, c);
            Tile& boardTile = board.tileAt({r + 1, c + 1});
            boardTile.type = tile.type;
            boardTile.lit = tile.lightIcon;
            if (tile.text.isEmpty()) {
                boardTile.army = 0;
            } else if (tile.type == TILE_SPAWN) {
                boardTile.spawnTeam = tile.text.at(0).unicode() - 'A' + 1;
            } else {
                boardTile.army = tile.text.toInt();
            }
        }
    }
    return board;
}

void MapCreatorWindow::fromBoard(const Board& board) {
    const int width = board.getWidth(), height = board.getHeight();
    map->realloc(width, height);
    bool hasBadTiles = false;
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            DisplayTile& tile = map->tileAt(r, c);
            const Tile& loadedTile = board.tileAt({r + 1, c + 1});
            tile.lightIcon = loadedTile.lit;
            if (loadedTile.occupier != -1) {
                tile.type = TILE_BLANK;
                tile.color = Qt::red;
                tile.text = "ERR";
                hasBadTiles = true;
                continue;
            }
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
            if (loadedTile.type == TILE_SPAWN) {
                unsigned team = loadedTile.spawnTeam;
                if (team > 0)
                    tile.text = QString(QLatin1Char('A' + team - 1));
                else
                    tile.text.clear();
            } else if (loadedTile.army != 0 || loadedTile.type == TILE_CITY)
                tile.text = QString::number(loadedTile.army);
            else
                tile.text.clear();
        }
    }
    if (hasBadTiles) {
        QTimer::singleShot(0, this, [this]() {
            QMessageBox::warning(this, "Warning",
                                 "Some tiles are not recognized. "
                                 "Please edit or truncate them before saving.");
        });
    }
}

const QString MapCreatorWindow::mapFileFilter =
    "All Supported Maps (*.lgmp *.lg *.json);;"
    "Official Generals.io Map (*.json);;"
    "LocalGen v5 (*.lg) - Legacy Format;;"
    "LocalGen v6 (*.lgmp) - Current Format;;";

void MapCreatorWindow::onOpenMap() {
    QString filename =
        QFileDialog::getOpenFileName(this, "Open Map", "", mapFileFilter);
    if (filename.isEmpty()) return;

    MapDocument doc;
    QString errMsg;
    if (filename.endsWith(".lg")) {
        doc.board = openMap_v5(filename, errMsg);
        if (errMsg.isEmpty()) {
            QFileInfo fileInfo(filename);
            doc.metadata.title = fileInfo.completeBaseName();
            doc.metadata.creationDateTime = fileInfo.birthTime();
        }
    } else if (filename.endsWith(".lgmp")) {
        doc = openMap_v6(filename, errMsg);
    } else if (filename.endsWith(".json")) {
        doc = openOfficialMap(filename, errMsg);
    } else {
        errMsg =
            "Unsupported file format. Please select a "
            ".lg, .lgmp, or .json file.";
    }

    if (!errMsg.isEmpty()) {
        QMessageBox::critical(this, "Error", errMsg);
        return;
    }

    this->fromBoard(doc.board);
    this->setMapMetadata(doc.metadata);

    map->fitCenter();
    widthSlider->setValue(map->mapWidth());
    heightSlider->setValue(map->mapHeight());
}

void MapCreatorWindow::onImportFromWeb() {
    bool ok;
    QString mapTitle = QInputDialog::getText(
        this, "Import from Generals.io",
        "Enter the map title:", QLineEdit::Normal, "", &ok);
    if (!ok || mapTitle.isEmpty()) return;

    auto encodedTitle = QUrl::toPercentEncoding(mapTitle);
    QNetworkRequest request(
        QUrl("https://generals.io/api/map?name=" + encodedTitle));
    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        "LocalGen/1.0 (+https://github.com/SZXC-WG/LocalGen-new)");

    if (!networkManager) {
        networkManager = new QNetworkAccessManager(this);
    }

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        // Check for HTTP errors
        int statusCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 404) {
            QMessageBox::critical(this, "Error",
                                  "The requested map does not exist.");
            return;
        }

        // Other network errors
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(
                this, "Network Error",
                "Failed to fetch map: " + reply->errorString());
            return;
        }

        // Finally: parse the response
        QByteArray data = reply->readAll();
        QString errMsg;
        MapDocument doc = openOfficialMap(data, errMsg);
        if (!errMsg.isEmpty()) {
            QMessageBox::critical(this, "Error", errMsg);
            return;
        }
        this->fromBoard(doc.board);
        this->setMapMetadata(doc.metadata);
        map->fitCenter();
        widthSlider->setValue(map->mapWidth());
        heightSlider->setValue(map->mapHeight());
    });
}

void MapCreatorWindow::onSaveMap() {
    QString filename =
        QFileDialog::getSaveFileName(this, "Save Map", "", mapFileFilter);
    if (filename.isEmpty()) return;

    MapDocument doc{currentMetadata(), toBoard()};

    QString errMsg;
    if (filename.endsWith(".lg")) {
        if (QMessageBox::warning(
                this, "Legacy Format Warning",
                "The .lg format does not support map metadata. Title, author, "
                "creation date, and description will not be saved.",
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Cancel) == QMessageBox::Cancel) {
            return;
        }
        saveMap_v5(filename, doc.board, errMsg);
    } else if (filename.endsWith(".lgmp")) {
        saveMap_v6(filename, doc, errMsg);
    } else {
        errMsg = "Unsupported file format. Please select a .lg or .lgmp file.";
    }

    if (!errMsg.isEmpty()) {
        QMessageBox::critical(this, "Error", errMsg);
    }
}
