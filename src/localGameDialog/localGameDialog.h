#ifndef LOCALGAMEDIALOG_H
#define LOCALGAMEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class LocalGameDialog;
}
QT_END_NAMESPACE

class LocalGameDialog : public QDialog {
    Q_OBJECT

   public:
    LocalGameDialog(QWidget* parent = nullptr);
    ~LocalGameDialog();

   public slots:
    void on_btnStartGame_clicked();
    void on_btnCancel_clicked();
    void on_spinBox_numPlayers_valueChanged(int);

   private:
    Ui::LocalGameDialog* ui;
};

#endif  // LOCALGAMEDIALOG_H
