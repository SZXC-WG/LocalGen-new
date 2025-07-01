#ifndef MAPCREATORWINDOW_H
#define MAPCREATORWINDOW_H

#include <QDialog>
#include <QKeyEvent>
#include <QPushButton>
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

   private:
    void setupToolbar();
    void updateToolButtonStyles();
    void keyPressEvent(QKeyEvent* event) override;

    MapWidget* map;
    QWidget* toolbar;
    QList<QPushButton*> toolButtons;
    int selectedToolIndex;

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