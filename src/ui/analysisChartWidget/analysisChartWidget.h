// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ANALYSISCHARTWIDGET_H
#define ANALYSISCHARTWIDGET_H

#include <QColor>
#include <QFrame>
#include <QList>
#include <QPointF>
#include <QPushButton>
#include <QString>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QValueAxis>
#include <vector>

#include "../leaderboardWidget/leaderboardWidget.h"
#include "core/utils.hpp"

struct PlayerAnalysisSeries {
    QLineSeries* series = nullptr;
    QList<QPointF> linearArmyHistory;
    QList<QPointF> linearLandHistory;
    QList<QPointF> logArmyHistory;
    QList<QPointF> logLandHistory;
};

class AnalysisChartWidget : public QFrame {
    Q_OBJECT

   public:
    explicit AnalysisChartWidget(QWidget* parent, int playerCount,
                                 const std::vector<QColor>& playerColors,
                                 const std::vector<QString>& playerNames);

    void updateAnalysis(const std::vector<LeaderboardRow>& rows);

   private:
    void refreshChart();
    void updateAxisRanges();

    QChart* chart = nullptr;
    QChartView* chartView = nullptr;
    QValueAxis* axisX = nullptr;
    QValueAxis* axisYLinear = nullptr;
    QLogValueAxis* axisYLog = nullptr;
    QAbstractButton* metricToggle = nullptr;
    QAbstractButton* scaleToggle = nullptr;

    bool showingLand = false;
    bool usingLogScale = false;
    int sampleCount = 0;
    army_t armyMax = 0;
    int landMax = 0;
    std::vector<PlayerAnalysisSeries> seriesData;
};

#endif  // ANALYSISCHARTWIDGET_H
