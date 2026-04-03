// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainWindow.h"

#include <QMessageBox>

#include "../localGameDialog/localGameDialog.h"
#include "../localGameWindow/localGameWindow.h"
#include "../mapCreatorWindow/mapCreatorWindow.h"
#include "ui_mainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_btnLocalGame_clicked() {
    // Keep this as an independent top-level window so it stays on its
    // current virtual desktop instead of following the main window.
    LocalGameDialog dialog(nullptr);
    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        // Use the same top-level window behavior for the in-game window.
        LocalGameWindow window(nullptr, dialog.config());
        if (window.layout() == nullptr) return;
        window.exec();
    }
}

void MainWindow::on_btnWebGame_clicked() {
    QMessageBox::information(this, "Not implemented",
                             "The web game mode is not available yet.");
}

void MainWindow::on_btnLoadReplay_clicked() {
    QMessageBox::information(
        this, "Not implemented",
        "The replay loading feature is not available yet.");
}

void MainWindow::on_btnCreateMap_clicked() {
    // Open the map creator as its own top-level window for desktop isolation.
    MapCreatorWindow window(nullptr);
    window.exec();
}
