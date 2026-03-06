#include <QApplication>
#include <QStyleHints>

#include "ui/mainWindow/mainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("windows11");
    QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    MainWindow window;
    window.show();
    return app.exec();
}
