#include <QApplication>

#include "ui/mainWindow/mainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("windows11");
    MainWindow window;
    window.show();
    return app.exec();
}
