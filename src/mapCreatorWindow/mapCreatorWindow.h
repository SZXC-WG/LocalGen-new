#ifndef MAPCREATORWINDOW_H
#define MAPCREATORWINDOW_H

#include <QDialog>
#include <QKeyEvent>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
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

   private:
    void setupToolbar();
    void updateToolButtonStyles();
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void repositionFloatingElements();

    MapWidget* map;
    QWidget* toolbar;
    QList<QPushButton*> toolButtons;
    int selectedToolIndex;
    QWidget* sliderContainer;
    QSlider* widthSlider;
    QSlider* heightSlider;

    enum class ToolType : std::uint8_t {
        MOUNTAIN = 0,
        LOOKOUT,
        OBSERVATORY,
        DESERT,
        SWAMP,
        CROWN,
        CITY,
        NEUTRAL,
        LIGHT,
        ERASE
    };
};

#endif  // MAPCREATORWINDOW_H