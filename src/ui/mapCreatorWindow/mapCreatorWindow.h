#ifndef MAPCREATORWINDOW_H
#define MAPCREATORWINDOW_H

#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QToolButton>
#include <QWidget>
#include <cstdint>

#include "../mapWidget/mapWidget.h"
#include "src/core/map.hpp"

class QNetworkAccessManager;

class MapCreatorWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MapCreatorWindow(QWidget* parent = nullptr);
    ~MapCreatorWindow();

    void onMapClicked(int r, int c);

   private slots:
    void onToolSelected();
    void setupSliders();
    void setupMetadataSidebar();
    void onOpenMap();
    void onSaveMap();
    void onImportFromWeb();

   private:
    void setupToolbar();
    void updateToolButtonStyles();
    void setupHintBar();
    void toggleMetadataSidebar();
    void updateMetadataSidebar(bool animate = false);
    void applyMetadataSidebarState();
    QRect metadataSidebarGeometry(bool expanded) const;
    QRect metadataSidebarButtonGeometry() const;
    void setMapMetadata(const MapMetadata& metadata);
    MapMetadata currentMetadata() const;
    void resetMapMetadata(
        const QString& mapName = QString(), const QString& author = QString(),
        const QDateTime& creationDateTime = QDateTime::currentDateTime(),
        const QString& description = QString());
    void updateHintBar();
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void repositionFloatingElements();

    // DisplayBoard <-> Board conversion
    Board toBoard() const;
    void fromBoard(const Board& board);

    QNetworkAccessManager* networkManager;

    static const QString mapFileFilter;

    MapWidget* map;
    QWidget* toolbar;
    QList<QPushButton*> toolButtons;
    QWidget* sidebarContainer;
    QWidget* sidebarPanel;
    QWidget* sidebarHeader;
    QLabel* sidebarTitleLabel;
    QToolButton* sidebarToggleButton;
    QWidget* metadataFormContainer;
    QLineEdit* mapTitleEdit;
    QLineEdit* authorEdit;
    QDateTimeEdit* creationDateEdit;
    QTextEdit* descriptionEdit;
    QPropertyAnimation* sidebarAnimation;
    bool metadataSidebarExpanded;

    QWidget* sliderContainer;
    QSlider* widthSlider;
    QSlider* heightSlider;
    QWidget* hintContainer;
    QLabel* hintLabel;
    QSpinBox* valueSpinBox;
    QLabel* valueLabel;
    QComboBox* teamComboBox;

    enum ToolType {
        MOUNTAIN = 0,
        LOOKOUT,
        OBSERVATORY,
        DESERT,
        SWAMP,
        SPAWN,
        CITY,
        NEUTRAL,
        LIGHT,
        ERASE
    } selectedTool;
};

#endif  // MAPCREATORWINDOW_H