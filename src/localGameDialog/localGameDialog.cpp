#include "localGameDialog.h"
#include "./ui_localGameDialog.h"

LocalGameDialog::LocalGameDialog(QWidget* parent) :
    QDialog(parent), ui(new Ui::LocalGameDialog) {
    ui->setupUi(this);
    // connect slots
    connect(ui->btnStartGame, &QPushButton::clicked, this, &LocalGameDialog::on_btnStartGame_clicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &LocalGameDialog::on_btnCancel_clicked);
}

LocalGameDialog::~LocalGameDialog() {
    delete ui;
}

void LocalGameDialog::on_btnStartGame_clicked() {
    this->done(QDialog::Accepted);
}

void LocalGameDialog::on_btnCancel_clicked() {
    this->done(QDialog::Rejected);
}
