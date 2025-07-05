#ifndef MAPCREATORWINDOW_H
#define MAPCREATORWINDOW_H

#include <QComboBox>
#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>
#include <cstdint>

#include "../mapWidget/mapWidget.h"

class MapCreatorWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MapCreatorWindow(QWidget* parent = nullptr);
    ~MapCreatorWindow();

    void onMapClicked(int r, int c);

   private slots:
    void onToolSelected();
    void setupSliders();
    void onOpenMap();
    void onSaveMap();

   private:
    void setupToolbar();
    void updateToolButtonStyles();
    void setupHintBar();
    void updateHintBar();
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void repositionFloatingElements();

    static const QString mapFileFilter;

    MapWidget* map;
    QWidget* toolbar;
    QList<QPushButton*> toolButtons;

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