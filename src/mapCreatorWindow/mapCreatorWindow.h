#ifndef MAPCREATORWINDOW_H
#define MAPCREATORWINDOW_H

#include <QDialog>

#include "../mapWidget/mapWidget.h"

class MapCreatorWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MapCreatorWindow(QWidget* parent = nullptr);
    ~MapCreatorWindow();

    void onMapClicked(int r, int c);

   private:
    MapWidget* map;
};

#endif  // MAPCREATORWINDOW_H