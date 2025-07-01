#ifndef MAPCREATORWINDOW_H
#define MAPCREATORWINDOW_H

#include <QDialog>

class MapCreatorWindow : public QDialog {
    Q_OBJECT

   public:
    explicit MapCreatorWindow(QWidget* parent = nullptr);
    ~MapCreatorWindow();

   private:
    // Private members for the map creator window
};

#endif  // MAPCREATORWINDOW_H