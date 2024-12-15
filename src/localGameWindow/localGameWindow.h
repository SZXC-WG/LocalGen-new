#ifndef LOCALGAMEWINDOW_H
#define LOCALGAMEWINDOW_H

#include <QDialog>
#include "../mapWidget/mapWidget.h"

class LocalGameWindow : public QDialog {
    Q_OBJECT

   public:
    LocalGameWindow(QWidget* parent = nullptr);
    ~LocalGameWindow();

   private:
    MapWidget* gameMap;
};

#endif  // LOCALGAMEWINDOW_H
