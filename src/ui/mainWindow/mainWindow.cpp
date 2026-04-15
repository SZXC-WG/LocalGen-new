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
    LocalGameDialog dialog(this);
    dialog.setPalette(this->palette());
    if (dialog.exec() == QDialog::Accepted) {
        LocalGameWindow window(this, dialog.config());
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
    MapCreatorWindow window(this);
    window.exec();
}
