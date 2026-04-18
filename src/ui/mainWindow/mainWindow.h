// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMainWindow>
class QContextMenuEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

   public slots:
    void on_btnLocalGame_clicked();
    void on_btnWebGame_clicked();
    void on_btnLoadReplay_clicked();
    void on_btnCreateMap_clicked();

   protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

   private:
    Ui::MainWindow* ui;
};
