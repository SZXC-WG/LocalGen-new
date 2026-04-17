// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localGameDialog.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QRandomGenerator>
#include <QStringList>
#include <algorithm>

#include "../utils/comboBoxPopupCompatibility.hpp"
#include "core/bot.h"
#include "core/map.hpp"
#include "ui_localGameDialog.h"

namespace {

constexpr int MapPathRole = Qt::UserRole;
constexpr int MapWidthRole = Qt::UserRole + 1;
constexpr int MapHeightRole = Qt::UserRole + 2;

struct LocalMapChoice {
    QString filePath;
    QString fileName;
    QString displayName;
    int width = 0;
    int height = 0;
};

QStringList toQStringList(const std::vector<std::string>& vec) {
    QStringList list;
    for (const auto& str : vec) {
        list.append(QString::fromStdString(str));
    }
    return list;
}

}  // namespace

LocalGameDialog::LocalGameDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::LocalGameDialog) {
    ui->setupUi(this);
    ComboBoxPopupCompatibility::configureForManagedPopup(ui->comboBox_gameMap);
    randomMapWidth = ui->spinBox_mapWidth->value();
    randomMapHeight = ui->spinBox_mapHeight->value();
    populateAvailableMaps();
    on_spinBox_numPlayers_valueChanged(ui->spinBox_numPlayers->value());
}

LocalGameDialog::~LocalGameDialog() { delete ui; }

LocalGameConfig LocalGameDialog::config() const {
    LocalGameConfig config;
    config.gameSpeed = ui->spinBox_gameSpeed->value();
    config.enableSounds = ui->checkBox_enableSounds->isChecked();
    config.showAnalysis = ui->checkBox_showAnalysis->isChecked();
    config.mapName = ui->comboBox_gameMap->currentText();
    config.mapFilePath =
        ui->comboBox_gameMap->currentData(MapPathRole).toString();
    config.mapWidth = ui->spinBox_mapWidth->value();
    config.mapHeight = ui->spinBox_mapHeight->value();
    auto& players = config.players;
    int numPlayers = ui->gridLayout_players->count();
    players.resize(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        QLayoutItem* item = ui->gridLayout_players->itemAt(i);
        QComboBox* combo = item->widget()->findChild<QComboBox*>();
        players[i] = combo->currentText();
    }
    return config;
}

void LocalGameDialog::on_btnStartGame_clicked() {
    this->done(QDialog::Accepted);
}

void LocalGameDialog::on_btnCancel_clicked() { this->done(QDialog::Rejected); }

void LocalGameDialog::populateAvailableMaps() {
    auto* mapCombo = ui->comboBox_gameMap;
    mapCombo->clear();
    mapCombo->addItem("Standard", QString());

    const QString standardLabel = mapCombo->itemText(0);
    QHash<QString, int> displayNameCounts;
    displayNameCounts.insert(standardLabel, 1);

    std::vector<LocalMapChoice> choices;
    QDir mapsDir(QDir(QCoreApplication::applicationDirPath()).filePath("maps"));
    if (mapsDir.exists()) {
        const QFileInfoList mapFiles = mapsDir.entryInfoList(
            QStringList{"*.lgmp"}, QDir::Files | QDir::NoSymLinks,
            QDir::Name | QDir::IgnoreCase);
        choices.reserve(mapFiles.size());

        for (const QFileInfo& fileInfo : mapFiles) {
            QString errMsg;
            MapDocument doc = openMap_v6(fileInfo.absoluteFilePath(), errMsg);
            if (!errMsg.isEmpty()) continue;
            if (doc.board.getWidth() <= 0 || doc.board.getHeight() <= 0)
                continue;

            LocalMapChoice choice;
            choice.filePath = fileInfo.absoluteFilePath();
            choice.fileName = fileInfo.fileName();
            choice.displayName = doc.metadata.title.trimmed();
            if (choice.displayName.isEmpty()) {
                choice.displayName = fileInfo.fileName();
            }
            choice.width = doc.board.getWidth();
            choice.height = doc.board.getHeight();
            choices.push_back(choice);
            ++displayNameCounts[choice.displayName];
        }
    }

    for (const LocalMapChoice& choice : choices) {
        QString displayName = choice.displayName;
        if (displayNameCounts.value(displayName) > 1) {
            displayName = QString("%1 (%2)").arg(displayName, choice.fileName);
        }

        const int index = mapCombo->count();
        mapCombo->addItem(displayName, choice.filePath);
        mapCombo->setItemData(index, choice.width, MapWidthRole);
        mapCombo->setItemData(index, choice.height, MapHeightRole);
    }

    QFontMetrics fm(mapCombo->font());
    int maxWidth = 0;
    for (int i = 0; i < mapCombo->count(); ++i)
        maxWidth =
            std::max(maxWidth, fm.horizontalAdvance(mapCombo->itemText(i)));
    mapCombo->view()->setMinimumWidth(maxWidth + 30);

    mapCombo->setCurrentIndex(0);
    on_comboBox_gameMap_currentIndexChanged(0);
}

void LocalGameDialog::on_spinBox_numPlayers_valueChanged(int numPlayers) {
    QGridLayout* layout = ui->gridLayout_players;
    int requiredCount = numPlayers;

    while (layout->count() > requiredCount) {
        QLayoutItem* item = layout->takeAt(layout->count() - 1);
        item->widget()->deleteLater();
        delete item;
    }
    if (layout->count() == requiredCount) return;

    const QStringList botNames = toQStringList(BotFactory::instance().list());
    const QFont& font = ui->labNumPlayers->font();
    const QFont& comboFont = ui->comboBox_gameMap->font();
    QRandomGenerator* rng = QRandomGenerator::global();

    const int columns = 2;
    while (layout->count() < requiredCount) {
        int index = layout->count();

        QFrame* cardFrame = new QFrame();
        cardFrame->setFrameShape(QFrame::StyledPanel);
        cardFrame->setFrameShadow(QFrame::Raised);

        QHBoxLayout* cardLayout = new QHBoxLayout(cardFrame);
        cardLayout->setContentsMargins(8, 4, 8, 4);

        QLabel* playerLabel = new QLabel(tr("P%1").arg(index + 1));
        playerLabel->setFont(font);

        QComboBox* playerCombo = new QComboBox();
        ComboBoxPopupCompatibility::configureForManagedPopup(playerCombo);
        if (index == 0) playerCombo->addItem("Human");
        for (const QString& botName : botNames) {
            playerCombo->addItem(botName);
        }
        playerCombo->setCurrentIndex(
            index == 0 ? 0 : rng->bounded(playerCombo->count()));
        playerCombo->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Preferred);
        playerCombo->setFont(comboFont);

        cardLayout->addWidget(playerLabel);
        cardLayout->addWidget(playerCombo);

        layout->addWidget(cardFrame, index / columns, index % columns);
    }
}

void LocalGameDialog::on_comboBox_gameMap_currentIndexChanged(int index) {
    if (index < 0) return;

    const QString mapFilePath =
        ui->comboBox_gameMap->itemData(index, MapPathRole).toString();

    updatingMapControls = true;
    if (mapFilePath.isEmpty()) {
        ui->spinBox_mapWidth->setEnabled(true);
        ui->spinBox_mapHeight->setEnabled(true);
        ui->spinBox_mapWidth->setValue(randomMapWidth);
        ui->spinBox_mapHeight->setValue(randomMapHeight);
    } else {
        ui->spinBox_mapWidth->setValue(
            ui->comboBox_gameMap->itemData(index, MapWidthRole).toInt());
        ui->spinBox_mapHeight->setValue(
            ui->comboBox_gameMap->itemData(index, MapHeightRole).toInt());
        ui->spinBox_mapWidth->setEnabled(false);
        ui->spinBox_mapHeight->setEnabled(false);
    }
    updatingMapControls = false;
}

void LocalGameDialog::on_spinBox_mapWidth_valueChanged(int value) {
    if (updatingMapControls || !ui->spinBox_mapWidth->isEnabled()) return;
    randomMapWidth = value;
}

void LocalGameDialog::on_spinBox_mapHeight_valueChanged(int value) {
    if (updatingMapControls || !ui->spinBox_mapHeight->isEnabled()) return;
    randomMapHeight = value;
}
