#include "mainWindow.h"
#include "./ui_mainWindow.h"
#include "../localGameDialog/localGameDialog.h"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // connect signals and slots
    connect(ui->btnLocalGame, &QPushButton::clicked, this, &MainWindow::on_btnLocalGame_clicked);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_btnLocalGame_clicked() {
    LocalGameDialog dialog(this);
    dialog.exec();
}
