// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

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
