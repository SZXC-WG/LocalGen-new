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

static void loadFonts() {
    const QStringList fontFileNames = {
        "Quicksand-Regular.ttf",
        "Quicksand-Medium.ttf",
        "Quicksand-Bold.ttf",
    };

    QStringList warnings;
    const QDir appDir(QCoreApplication::applicationDirPath());
    for (const QString& fontFileName : fontFileNames) {
        const QString fontPath = appDir.filePath("fonts/" + fontFileName);
        const QFileInfo fontFile(fontPath);
        if (!fontFile.exists()) {
            warnings
                << QString("Required font file not found:\n%1").arg(fontPath);
            continue;
        }

        const int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId < 0) {
            warnings << QString("Failed to load font file:\n%1").arg(fontPath);
            continue;
        }

        const QStringList fontFamilies =
            QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.contains("Quicksand")) {
            warnings << QString(
                            "Bundled font family should include \"Quicksand\", "
                            "but got: %1")
                            .arg(fontFamilies.join(", "));
        }
    }

    if (!warnings.isEmpty()) {
        QMessageBox::warning(nullptr, "Font Warning", warnings.join("\n\n"));
    }
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("windows11");
    QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    loadFonts();
    MainWindow window;
    window.show();
    return app.exec();
}
