// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMessageBox>
#include <QStringList>
#include <QStyleHints>

#include "ui/mainWindow/mainWindow.h"

namespace {

void loadFonts() {
    const QString fontPath = QDir(QCoreApplication::applicationDirPath())
                                 .filePath("fonts/Quicksand[wght].ttf");
    const QFileInfo fontFile(fontPath);
    if (!fontFile.exists()) {
        QMessageBox::warning(
            nullptr, "Font Warning",
            QString("Required font file not found:\n%1").arg(fontPath));
        return;
    }

    const int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId < 0) {
        QMessageBox::warning(
            nullptr, "Font Warning",
            QString("Failed to load font file:\n%1").arg(fontPath));
        return;
    }

    const QString fontFamily =
        QFontDatabase::applicationFontFamilies(fontId).at(0);
    if (fontFamily != "Quicksand") {
        QMessageBox::warning(
            nullptr, "Font Warning",
            QString("Bundled font family should be \"Quicksand\", but got: %1")
                .arg(fontFamily));
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("windows11");
    QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    loadFonts();
    MainWindow window;
    window.show();
    return app.exec();
}
