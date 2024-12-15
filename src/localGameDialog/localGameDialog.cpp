#include "localGameDialog.h"
#include "ui_localGameDialog.h"

LocalGameDialog::LocalGameDialog(QWidget* parent) :
    QDialog(parent), ui(new Ui::LocalGameDialog) {
    ui->setupUi(this);
    on_spinBox_numPlayers_valueChanged(ui->spinBox_numPlayers->value());
}

LocalGameDialog::~LocalGameDialog() {
    delete ui;
}

LocalGameConfig LocalGameDialog::config() const {
    LocalGameConfig config;
    config.gameSpeed = ui->spinBox_gameSpeed->value();
    config.enableSounds = ui->checkBox_enableSounds->isChecked();
    config.showAnalysis = ui->checkBox_showAnalysis->isChecked();
    config.mapName = ui->comboBox_gameMap->currentText();
    config.mapWidth = ui->spinBox_mapWidth->value();
    config.mapHeight = ui->spinBox_mapHeight->value();
    int numPlayers = ui->spinBox_numPlayers->value();
    auto& players = config.players;
    players.resize(numPlayers);
    QLayout* layout = ui->groupBox_players->layout();
    for(int i = 0; i < numPlayers; ++i) {
        QWidget* playerWidget = layout->itemAt(i + 1)->widget();
        players[i].name = playerWidget->findChild<QComboBox*>()->currentText();
        players[i].visible = playerWidget->findChild<QCheckBox*>()->isChecked();
    }
    return config;
}

void LocalGameDialog::on_btnStartGame_clicked() {
    this->done(QDialog::Accepted);
}

void LocalGameDialog::on_btnCancel_clicked() {
    this->done(QDialog::Rejected);
}

void LocalGameDialog::on_spinBox_numPlayers_valueChanged(int numPlayers) {
    QLayout* layout = ui->groupBox_players->layout();
    int requiredCount = numPlayers + 1;
    while(layout->count() > requiredCount) {
        layout->takeAt(layout->count() - 1)->widget()->deleteLater();
    }
    // Bot names from v5, for demonstration only.
    const QStringList botNames = { "ktqBot", "lcwBot", "smartRandomBot", "xrzBot", "zlyBot" };
    const QFont& font = ui->labNumPlayers->font();
    const QFont& comboFont = ui->comboBox_gameMap->font();
    while(layout->count() < requiredCount) {
        QLabel* playerLabel = new QLabel(tr("Player %1").arg(layout->count()));
        playerLabel->setFont(font);
        QComboBox* playerCombo = new QComboBox();
        if(layout->count() == 1) playerCombo->addItem(tr("Human"));
        playerCombo->addItems(botNames);
        playerCombo->setCurrentIndex(0);
        playerCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        playerCombo->setStyleSheet("color: teal;");
        playerCombo->setFont(comboFont);
        QCheckBox* visibleChkbox = new QCheckBox(tr("Visible"));
        visibleChkbox->setChecked(layout->count() == 1);
        visibleChkbox->setFont(font);
        QHBoxLayout* playerLayout = new QHBoxLayout();
        playerLayout->addWidget(playerLabel);
        playerLayout->addWidget(playerCombo);
        playerLayout->addWidget(visibleChkbox);
        playerLayout->setContentsMargins(0, 0, 0, 0);
        QWidget* playerWidget = new QWidget();
        playerWidget->setLayout(playerLayout);
        layout->addWidget(playerWidget);
    }
}
