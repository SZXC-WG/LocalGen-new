// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainWindow.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>

#include "../localGameDialog/localGameDialog.h"
#include "../localGameWindow/localGameWindow.h"
#include "../mapCreatorWindow/mapCreatorWindow.h"
#include "ui_mainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setWindowTitle("LocalGen " APP_VERSION_STRING);
    ui->labVersion->setText("version " APP_VERSION_STRING);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_btnLocalGame_clicked() {
    LocalGameDialog dialog(this);
    dialog.setPalette(this->palette());
    if (dialog.exec() == QDialog::Accepted) {
        LocalGameWindow window(nullptr, dialog.config());
        if (window.layout() == nullptr) return;
        window.setWindowIcon(this->windowIcon());
        this->hide();
        window.exec();
        this->show();
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
    MapCreatorWindow window(nullptr);
    window.setWindowIcon(this->windowIcon());
    this->hide();
    window.exec();
    this->show();
}

const QString aboutText = QStringLiteral(
    "<h3>Local Generals.io</h3>"
    "<p>Version " APP_VERSION_STRING
    "</p>"
    "<p>An unofficial local clone of generals.io.</p>"
    "<p>Copyright &copy; 2026 SZXC Work Group</p>"
    "<p>This program is free software: you can redistribute it and/or modify"
    " it under the terms of the GNU General Public License as published by"
    " the Free Software Foundation, either version 3 of the License, or"
    " (at your option) any later version.</p>"
    "<p>This program is distributed in the hope that it will be useful,"
    " but WITHOUT ANY WARRANTY; without even the implied warranty of"
    " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the"
    " GNU General Public License for more details.</p>"
    "<p>You should have received a copy of the GNU General Public License"
    " along with this program. If not, see <a "
    "href=\"https://www.gnu.org/licenses/\">https://www.gnu.org/licenses/</"
    "a>.</p>"
    "<p>The source code for LocalGen is available at "
    "<a href=\"https://github.com/SZXC-WG/LocalGen-new\">"
    "https://github.com/SZXC-WG/LocalGen-new</a>.</p>");

void MainWindow::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.addAction("About", this, [this]() {
        QMessageBox::about(this, "About LocalGen", aboutText);
    });
    menu.addAction("About Qt", qApp, &QApplication::aboutQt);
    menu.exec(event->globalPos());
}
