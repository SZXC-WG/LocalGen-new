#include "localGameDialog.h"

#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QHash>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QListView>
#include <QPointer>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QStringList>
#include <QVBoxLayout>
#include <QWheelEvent>

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

class MapComboPopupListView final : public QListView {
   public:
    explicit MapComboPopupListView(QWidget* parent = nullptr)
        : QListView(parent) {}

    bool scrollFromWheel(QWheelEvent* event) {
        if (event == nullptr) return false;

        // WSL/Linux can miss the default popup scrolling path, so we drive the
        // popup list directly through its scroll bar.
        QScrollBar* scrollBar = verticalScrollBar();
        if (scrollBar == nullptr || scrollBar->maximum() <= scrollBar->minimum()
            || !scrollBar->isEnabled()) {
            return false;
        }

        int delta = 0;
        if (!event->pixelDelta().isNull()) {
            delta = event->pixelDelta().y();
        } else {
            angleDeltaRemainder += event->angleDelta().y();
            const int steps = angleDeltaRemainder / 120;
            angleDeltaRemainder -= steps * 120;
            delta = steps * scrollBar->singleStep();
        }

        if (delta == 0) return false;

        scrollBar->setValue(scrollBar->value() - delta);
        event->accept();
        return true;
    }

   protected:
    bool viewportEvent(QEvent* event) override {
        if (event->type() == QEvent::Wheel &&
            scrollFromWheel(static_cast<QWheelEvent*>(event))) {
            return true;
        }
        return QListView::viewportEvent(event);
    }

    void wheelEvent(QWheelEvent* event) override {
        if (scrollFromWheel(event)) return;
        QListView::wheelEvent(event);
    }

   private:
    int angleDeltaRemainder = 0;
};

class MapComboBox final : public QComboBox {
   public:
    explicit MapComboBox(QWidget* parent = nullptr) : QComboBox(parent) {}

    ~MapComboBox() override {
        if (qApp != nullptr) {
            qApp->removeEventFilter(this);
        }
    }

   protected:
    void showPopup() override {
        if (count() == 0) return;

        ensurePopup();
        syncPopup();
        popupFrame->show();
        popupFrame->raise();
        popupList->setFocus(Qt::PopupFocusReason);
        qApp->installEventFilter(this);
    }

    void hidePopup() override {
        if (popupFrame != nullptr) {
            popupFrame->hide();
        }
        if (qApp != nullptr) {
            qApp->removeEventFilter(this);
        }
        QComboBox::hidePopup();
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (popupFrame == nullptr || popupList == nullptr || !popupFrame->isVisible()) {
            return QComboBox::eventFilter(watched, event);
        }

        QWidget* watchedWidget = qobject_cast<QWidget*>(watched);
        const bool insidePopup =
            watchedWidget != nullptr &&
            (watchedWidget == popupFrame || watchedWidget == popupList ||
             watchedWidget == popupList->viewport() ||
             popupFrame->isAncestorOf(watchedWidget));
        const bool insideCombo =
            watchedWidget != nullptr &&
            (watchedWidget == this || isAncestorOf(watchedWidget));

        switch (event->type()) {
            case QEvent::Wheel:
                if (insidePopup) return QComboBox::eventFilter(watched, event);
                return popupList->scrollFromWheel(static_cast<QWheelEvent*>(event));
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick:
                if (!insidePopup && !insideCombo) hidePopup();
                break;
            case QEvent::KeyPress:
                if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
                    hidePopup();
                    return true;
                }
                break;
            case QEvent::WindowDeactivate:
                hidePopup();
                break;
            default: break;
        }

        return QComboBox::eventFilter(watched, event);
    }

   private:
    void ensurePopup() {
        if (popupFrame != nullptr) return;

        // We render the popup inside the dialog instead of using Qt's default
        // combo popup so wheel events stay inside the application on WSL/Linux.
        popupFrame = new QFrame(window());
        popupFrame->setFrameShape(QFrame::StyledPanel);
        popupFrame->setLineWidth(1);

        auto* layout = new QVBoxLayout(popupFrame);
        layout->setContentsMargins(0, 0, 0, 0);

        popupList = new MapComboPopupListView(popupFrame);
        popupList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        popupList->setSelectionMode(QAbstractItemView::SingleSelection);
        popupList->setSelectionBehavior(QAbstractItemView::SelectRows);
        popupList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        popupList->setFont(font());
        popupList->setStyleSheet(styleSheet());
        layout->addWidget(popupList);

        connect(popupList, &QListView::clicked, this,
                [this](const QModelIndex& index) {
                    if (index.isValid()) {
                        setCurrentIndex(index.row());
                    }
                    hidePopup();
                });
        connect(popupList, &QListView::activated, this,
                [this](const QModelIndex& index) {
                    if (index.isValid()) {
                        setCurrentIndex(index.row());
                    }
                    hidePopup();
                });
    }

    void syncPopup() {
        popupList->setModel(model());
        popupList->setCurrentIndex(model()->index(currentIndex(), 0));
        popupList->scrollTo(popupList->currentIndex(),
                            QAbstractItemView::PositionAtCenter);

        const int rowHeight =
            qMax(fontMetrics().height() + 8, popupList->sizeHintForRow(0));
        const int visibleRows = qMax(1, qMin(8, count()));
        const int popupHeight = visibleRows * rowHeight + popupFrame->frameWidth() * 2;
        const int popupWidth = qMax(width(), popupList->sizeHintForColumn(0) + 32);

        QWidget* hostWindow = window();
        const QRect hostRect = hostWindow->rect();
        const QPoint below =
            hostWindow->mapFromGlobal(mapToGlobal(QPoint(0, height())));
        const QPoint above = hostWindow->mapFromGlobal(mapToGlobal(QPoint(0, 0)));

        int x = below.x();
        int y = below.y();
        if (x + popupWidth > hostRect.width()) {
            x = qMax(0, hostRect.width() - popupWidth);
        }
        if (y + popupHeight > hostRect.height() && above.y() - popupHeight >= 0) {
            y = above.y() - popupHeight;
        } else if (y + popupHeight > hostRect.height()) {
            y = qMax(0, hostRect.height() - popupHeight);
        }

        popupFrame->setGeometry(x, y, popupWidth, popupHeight);
    }

    QPointer<QFrame> popupFrame;
    QPointer<MapComboPopupListView> popupList;
};

QComboBox* replaceMapComboBox(QComboBox* comboBox) {
    if (comboBox == nullptr) return nullptr;

    QWidget* parent = comboBox->parentWidget();
    auto* replacement = new MapComboBox(parent);
    replacement->setObjectName(comboBox->objectName());
    replacement->setFont(comboBox->font());
    replacement->setStyleSheet(comboBox->styleSheet());
    replacement->setSizePolicy(comboBox->sizePolicy());

    if (parent != nullptr && parent->layout() != nullptr) {
        parent->layout()->replaceWidget(comboBox, replacement);
    }
    comboBox->hide();
    comboBox->deleteLater();
    replacement->show();
    return replacement;
}

}  // namespace

LocalGameDialog::LocalGameDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::LocalGameDialog) {
    // Promote this dialog to a regular top-level window so desktop switching
    // treats it like the main window.
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::Window;
    flags &= ~Qt::Dialog;
    setWindowFlags(flags);

    ui->setupUi(this);
    ui->comboBox_gameMap = replaceMapComboBox(ui->comboBox_gameMap);
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
    int numPlayers = ui->spinBox_numPlayers->value();
    auto& players = config.players;
    players.resize(numPlayers);
    QLayout* layout = ui->groupBox_players->layout();
    for (int i = 0; i < numPlayers; ++i) {
        QWidget* playerWidget = layout->itemAt(i + 1)->widget();
        players[i] = playerWidget->findChild<QComboBox*>()->currentText();
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

    mapCombo->setCurrentIndex(0);
    on_comboBox_gameMap_currentIndexChanged(0);
}

void LocalGameDialog::on_spinBox_numPlayers_valueChanged(int numPlayers) {
    QLayout* layout = ui->groupBox_players->layout();
    int requiredCount = numPlayers + 1;
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
    while (layout->count() < requiredCount) {
        QLabel* playerLabel = new QLabel(tr("Player %1").arg(layout->count()));
        playerLabel->setFont(font);
        QComboBox* playerCombo = new QComboBox();
        if (layout->count() == 1) playerCombo->addItem(tr("Human"));
        playerCombo->addItems(botNames);
        playerCombo->setCurrentIndex(
            layout->count() == 1 ? 0 : rng->bounded(playerCombo->count()));
        playerCombo->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Preferred);
        playerCombo->setStyleSheet("color: teal;");
        playerCombo->setFont(comboFont);
        QHBoxLayout* playerLayout = new QHBoxLayout();
        playerLayout->addWidget(playerLabel);
        playerLayout->addWidget(playerCombo);
        playerLayout->setContentsMargins(0, 0, 0, 0);
        QWidget* playerWidget = new QWidget();
        playerWidget->setLayout(playerLayout);
        layout->addWidget(playerWidget);
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
