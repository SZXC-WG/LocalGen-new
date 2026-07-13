// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QFrame>
#include <QList>
#include <QPointF>
#include <QSlider>
#include <QString>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QValueAxis>
#include <vector>

#include "core/game.hpp"

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
    explicit AnalysisChartWidget(QWidget* parent, size_t playerCount,
                                 const std::vector<QColor>& playerColors,
                                 const std::vector<QString>& playerNames);

    void updateAnalysis(const std::vector<RankItem>& rank);

   private:
    void refreshChart();
    void updateAxisRanges();

    QChart* chart = nullptr;
    QChartView* chartView = nullptr;
    QValueAxis* axisX = nullptr;
    QValueAxis* axisYLinear = nullptr;
    QLogValueAxis* axisYLog = nullptr;
    QSlider* metricSwitch = nullptr;
    QSlider* scaleSwitch = nullptr;

    bool showingLand = false;
    bool usingLogScale = false;
    int sampleCount = 0;
    army_t armyMax = 0;
    int landMax = 0;
    std::vector<PlayerAnalysisSeries> seriesData;
};
