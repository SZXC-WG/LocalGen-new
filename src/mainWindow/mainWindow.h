#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class LocalGameWindow;
class MapCreatorWindow;
class QCloseEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

   protected:
    void closeEvent(QCloseEvent* event) override;

   public slots:
    void on_btnLocalGame_clicked();
    void on_btnWebGame_clicked();
    void on_btnLoadReplay_clicked();
    void on_btnCreateMap_clicked();

   private:
    Ui::MainWindow* ui;
    QList<QWidget*> childWindows;
};

#endif  // MAINWINDOW_H
