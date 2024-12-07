#include "localGameDialog.h"
#include "./ui_localGameDialog.h"

LocalGameDialog::LocalGameDialog(QWidget* parent) :
    QDialog(parent), ui(new Ui::LocalGameDialog) {
    ui->setupUi(this);
}

LocalGameDialog::~LocalGameDialog() {
    delete ui;
}
