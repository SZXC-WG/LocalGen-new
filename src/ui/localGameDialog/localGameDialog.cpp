// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localGameDialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
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
#include <QScrollBar>
#include <QStringList>

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
    int width, height;
};

QStringList toQStringList(const std::vector<std::string>& vec) {
    QStringList list;
    list.reserve(vec.size());
    for (const auto& str : vec) {
        list.append(QString::fromStdString(str));
    }
    return list;
}

}  // namespace

LocalGameDialog::LocalGameDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::LocalGameDialog) {
    ui->setupUi(this);
    connect(ui->checkBox_crystalClear, &QCheckBox::toggled, this,
            [this](bool checked) {
                if (checked) ui->checkBox_mistyVeil->setChecked(false);
            });
    connect(ui->checkBox_mistyVeil, &QCheckBox::toggled, this,
            [this](bool checked) {
                if (checked) ui->checkBox_crystalClear->setChecked(false);
            });
    connect(ui->checkBox_silentWar, &QCheckBox::toggled, this,
            [this](bool checked) {
                ui->checkBox_showAnalysis->setEnabled(!checked);
                ui->checkBox_showAnalysis->setToolTip(
                    checked ? tr("Analysis is hidden while Silent War is on.")
                            : QString());
            });
    ComboBoxPopupCompatibility::configureForManagedPopup(ui->comboBox_gameMap);
    populateAvailableMaps();
    on_spinBox_numPlayers_valueChanged(ui->spinBox_numPlayers->value());
}

LocalGameDialog::~LocalGameDialog() { delete ui; }

LocalGameConfig LocalGameDialog::config() const {
    LocalGameConfig config{
        ui->spinBox_gameSpeed->value(),
        ui->checkBox_enableSounds->isChecked(),
        ui->checkBox_showAnalysis->isChecked(),
        ui->comboBox_gameMap->currentData(MapPathRole).toString(),
        ui->spinBox_mapWidth->value(),
        ui->spinBox_mapHeight->value(),
        {}};
    config.gameConfig.watchtower = ui->checkBox_watchtower->isChecked();
    config.gameConfig.mistyVeil = ui->checkBox_mistyVeil->isChecked();
    config.gameConfig.crystalClear = ui->checkBox_crystalClear->isChecked();
    config.gameConfig.fadingSmog = ui->checkBox_fadingSmog->isChecked();
    config.gameConfig.leapfrog = ui->checkBox_leapfrog->isChecked();
    config.gameConfig.cityState =
        ui->checkBox_cityState->isChecked() && config.mapFilePath.isEmpty();
    config.gameConfig.defenseless = ui->checkBox_defenseless->isChecked();
    config.gameConfig.slippery = ui->checkBox_slippery->isChecked();
    config.gameConfig.silentWar = ui->checkBox_silentWar->isChecked();
    config.showAnalysis = config.showAnalysis && !config.gameConfig.silentWar;

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

void LocalGameDialog::on_btnStartGame_clicked() { done(QDialog::Accepted); }
void LocalGameDialog::on_btnCancel_clicked() { done(QDialog::Rejected); }

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

            LocalMapChoice choice{fileInfo.absoluteFilePath(),
                                  fileInfo.fileName(),
                                  doc.metadata.title.trimmed(),
                                  doc.board.getWidth(), doc.board.getHeight()};
            if (choice.displayName.isEmpty()) {
                choice.displayName = choice.fileName;
            }
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

    auto* popupView = mapCombo->view();
    popupView->setMinimumWidth(
        popupView->sizeHintForColumn(0) +
        popupView->verticalScrollBar()->sizeHint().width());

    mapCombo->setCurrentIndex(0);
    on_comboBox_gameMap_currentIndexChanged(0);
}

void LocalGameDialog::on_spinBox_numPlayers_valueChanged(int numPlayers) {
    QGridLayout* layout = ui->gridLayout_players;

    while (layout->count() > numPlayers) {
        QLayoutItem* item = layout->takeAt(layout->count() - 1);
        item->widget()->deleteLater();
        delete item;
    }
    const QStringList botNames = toQStringList(BotFactory::instance().list());
    const QFont& font = ui->labNumPlayers->font();
    const QFont& comboFont = ui->comboBox_gameMap->font();
    QRandomGenerator* rng = QRandomGenerator::global();

    const int columns = 2;
    while (layout->count() < numPlayers) {
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
        playerCombo->addItems(botNames);
        playerCombo->setCurrentIndex(
            index == 0 ? 0 : rng->bounded(playerCombo->count()));
        playerCombo->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Preferred);
        playerCombo->setFont(comboFont);

        cardLayout->addWidget(playerLabel);
        cardLayout->addWidget(playerCombo);

        layout->addWidget(cardFrame, index / columns, index % columns);
        cardFrame->show();
    }

    layout->invalidate();
    ui->scrollArea_players->setFixedHeight(
        std::min(layout->sizeHint().height(), 180));
    ui->scrollArea_players->setMinimumWidth(
        layout->minimumSize().width() +
        ui->scrollArea_players->verticalScrollBar()->sizeHint().width());
}

void LocalGameDialog::on_comboBox_gameMap_currentIndexChanged(int index) {
    if (index < 0) return;

    const bool isRandom =
        ui->comboBox_gameMap->itemData(index, MapPathRole).toString().isEmpty();
    ui->spinBox_mapWidth->setEnabled(isRandom);
    ui->spinBox_mapHeight->setEnabled(isRandom);
    ui->checkBox_cityState->setEnabled(isRandom);

    if (!isRandom) {
        ui->spinBox_mapWidth->setValue(
            ui->comboBox_gameMap->itemData(index, MapWidthRole).toInt());
        ui->spinBox_mapHeight->setValue(
            ui->comboBox_gameMap->itemData(index, MapHeightRole).toInt());
    }
}
