#include "mainWindow.h"

#include <QMessageBox>
#include <QCloseEvent>

#include "../localGameDialog/localGameDialog.h"
#include "../localGameWindow/localGameWindow.h"
#include "../mapCreatorWindow/mapCreatorWindow.h"
#include "ui_mainWindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    for (auto* w : childWindows) {
        w->close();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::on_btnLocalGame_clicked() {
    LocalGameDialog dialog(this);
    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        LocalGameWindow* window = new LocalGameWindow(nullptr, dialog.config());
        childWindows.append(window);
        connect(window, &QObject::destroyed, this, [this, window]() {
            childWindows.removeOne(window);
        });
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
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
    MapCreatorWindow* window = new MapCreatorWindow(nullptr);
    childWindows.append(window);
    connect(window, &QObject::destroyed, this, [this, window]() {
        childWindows.removeOne(window);
    });
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
}
