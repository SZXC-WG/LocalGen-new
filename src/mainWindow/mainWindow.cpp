#include "mainWindow.h"
#include "ui_mainWindow.h"
#include "../localGameDialog/localGameDialog.h"
#include "../localGameWindow/localGameWindow.h"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_btnLocalGame_clicked() {
    LocalGameDialog* dialog = new LocalGameDialog(this);
    int result = dialog->exec();
    if(result == QDialog::Accepted) {
        LocalGameWindow* window = new LocalGameWindow(this, dialog->config());
        window->exec();
    }
}
