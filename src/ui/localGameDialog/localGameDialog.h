#ifndef LOCALGAMEDIALOG_H
#define LOCALGAMEDIALOG_H

#include <QDialog>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class LocalGameDialog;
}
QT_END_NAMESPACE

struct LocalGameConfig {
    int gameSpeed;
    bool enableSounds, showAnalysis;
    QString mapName;
    QString mapFilePath;
    int mapWidth, mapHeight;
    QStringList players;
};

class LocalGameDialog : public QDialog {
    Q_OBJECT

   public:
    LocalGameDialog(QWidget* parent = nullptr);
    ~LocalGameDialog();

    LocalGameConfig config() const;

   public slots:
    void on_btnStartGame_clicked();
    void on_btnCancel_clicked();
    void on_spinBox_numPlayers_valueChanged(int);
    void on_comboBox_gameMap_currentIndexChanged(int);
    void on_spinBox_mapWidth_valueChanged(int);
    void on_spinBox_mapHeight_valueChanged(int);

   private:
    void populateAvailableMaps();

    Ui::LocalGameDialog* ui;
    int randomMapWidth = 20;
    int randomMapHeight = 20;
    bool updatingMapControls = false;
};

#endif  // LOCALGAMEDIALOG_H
