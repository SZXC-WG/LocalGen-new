#ifndef LOCALGAMEWINDOW_H
#define LOCALGAMEWINDOW_H

#include <QDialog>

#include "../localGameDialog/localGameDialog.h"
#include "../mapWidget/mapWidget.h"

class LocalGameWindow : public QDialog {
    Q_OBJECT

   public:
    explicit LocalGameWindow(QWidget* parent, const LocalGameConfig& config);
    ~LocalGameWindow();

   private:
    MapWidget* gameMap;
};

#endif  // LOCALGAMEWINDOW_H
