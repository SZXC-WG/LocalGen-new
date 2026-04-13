// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "analysisChartWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtCharts/QLegend>
#include <algorithm>
#include <cmath>

namespace {

double adjustedAxisMax(double value) {
    if (value <= 1.0) {
        return 1.0;
    }

    return value + std::max(1.0, value * 0.08);
}

void styleAnalysisAxis(QAbstractAxis* axis, const QColor& foreground,
                       const QColor& gridColor) {
    axis->setLabelsFont(QFont("Quicksand", 9, QFont::Medium));
    axis->setTitleFont(QFont("Quicksand", 10, QFont::DemiBold));
    axis->setLabelsColor(foreground);
    axis->setTitleBrush(QBrush(foreground));
    axis->setLinePenColor(foreground);
    axis->setGridLineColor(gridColor);
    axis->setMinorGridLineColor(gridColor);
}

QPushButton* createToggleButton(const QString& text, QWidget* parent) {
    QPushButton* button = new QPushButton(text, parent);
    button->setCheckable(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(QFont("Quicksand", 10, QFont::Bold));
    QPalette pal = button->palette();
    pal.setColor(QPalette::ButtonText, Qt::white);
    button->setPalette(pal);
    return button;
}

qreal smoothedLinearValue(const QList<QPointF>& history, qreal rawValue) {
    constexpr qreal smoothingAlpha = 0.65;
    if (history.isEmpty() || rawValue <= 0.0) {
        return rawValue;
    }
    return history.constLast().y() +
           smoothingAlpha * (rawValue - history.constLast().y());
}

qreal smoothedLogValue(const QList<QPointF>& history, qreal rawValue) {
    constexpr qreal smoothingAlpha = 0.65;
    const qreal rawLogValue = std::log10(std::max<qreal>(1.0, rawValue));
    if (history.isEmpty() || rawValue <= 0.0) {
        return std::pow(static_cast<qreal>(10.0), rawLogValue);
    }
    const qreal previousSmoothedLog =
        std::log10(std::max<qreal>(1.0, history.constLast().y()));
    const qreal smoothedLog =
        previousSmoothedLog +
        smoothingAlpha * (rawLogValue - previousSmoothedLog);
    return std::pow(static_cast<qreal>(10.0), smoothedLog);
}

void appendAnalysisPoint(QList<QPointF>& history, qreal x, qreal y) {
    history.append(QPointF(x, y));
}

void appendAnalysisHistories(PlayerAnalysisSeries& series, qreal stepValue,
                             qreal armyValue, qreal landValue) {
    appendAnalysisPoint(
        series.linearArmyHistory, stepValue,
        smoothedLinearValue(series.linearArmyHistory, armyValue));
    appendAnalysisPoint(
        series.linearLandHistory, stepValue,
        smoothedLinearValue(series.linearLandHistory, landValue));
    appendAnalysisPoint(series.logArmyHistory, stepValue,
                        smoothedLogValue(series.logArmyHistory, armyValue));
    appendAnalysisPoint(series.logLandHistory, stepValue,
                        smoothedLogValue(series.logLandHistory, landValue));
}

const QList<QPointF>& historyForMode(const PlayerAnalysisSeries& series,
                                     bool showLand, bool useLogScale) {
    if (useLogScale) {
        return showLand ? series.logLandHistory : series.logArmyHistory;
    }
    return showLand ? series.linearLandHistory : series.linearArmyHistory;
}

QChart* createChart(const QString& title, QValueAxis*& outAxisX) {
    const QColor panelBackground(20, 23, 28, 0);
    const QColor plotBackground(32, 37, 44, 160);
    const QColor foreground(240, 244, 248);
    const QColor gridColor(240, 244, 248, 64);

    QChart* chart = new QChart();
    chart->setTitle(title);
    chart->setTitleFont(QFont("Quicksand", 11, QFont::Bold));
    chart->setTitleBrush(QBrush(foreground));
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setBackgroundBrush(QBrush(panelBackground));
    chart->setBackgroundPen(Qt::NoPen);
    chart->setBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(plotBackground));
    chart->setPlotAreaBackgroundPen(QPen(gridColor));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setMargins(QMargins(10, 10, 10, 10));
    chart->legend()->hide();

    outAxisX = new QValueAxis(chart);
    outAxisX->setTitleText("Step");
    outAxisX->setLabelFormat("%.0f");
    outAxisX->setTickCount(6);
    outAxisX->setMinorTickCount(0);
    outAxisX->setRange(0.0, 1.0);
    styleAnalysisAxis(outAxisX, foreground, gridColor);
    chart->addAxis(outAxisX, Qt::AlignBottom);

    return chart;
}

}  // namespace

AnalysisChartWidget::AnalysisChartWidget(
    QWidget* parent, int playerCount, const std::vector<QColor>& playerColors,
    const std::vector<QString>& playerNames)
    : QFrame(parent) {
    const QColor foreground(240, 244, 248);
    const QColor gridColor(240, 244, 248, 64);

    setAutoFillBackground(true);
    QPalette widgetPal = palette();
    widgetPal.setColor(QPalette::Window, QColor(18, 21, 26, 232));
    setPalette(widgetPal);
    setFrameShape(QFrame::NoFrame);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->addStretch(1);

    metricToggle = createToggleButton("Switch to Land", this);
    controlsLayout->addWidget(metricToggle);

    scaleToggle = createToggleButton("Switch to Log", this);
    controlsLayout->addWidget(scaleToggle);
    mainLayout->addLayout(controlsLayout);

    chart = createChart("Army Trend", axisX);

    axisYLinear = new QValueAxis(chart);
    axisYLinear->setLabelFormat("%.0f");
    axisYLinear->setTickCount(6);
    axisYLinear->setMinorTickCount(0);
    axisYLinear->setRange(0.0, 1.0);
    styleAnalysisAxis(axisYLinear, foreground, gridColor);
    chart->addAxis(axisYLinear, Qt::AlignLeft);

    axisYLog = new QLogValueAxis(chart);
    axisYLog->setLabelFormat("%.0f");
    axisYLog->setBase(10.0);
    axisYLog->setMinorTickCount(8);
    axisYLog->setRange(1.0, 10.0);
    styleAnalysisAxis(axisYLog, foreground, gridColor);

    chartView = new QChartView(chart, this);
    chartView->setFocusPolicy(Qt::NoFocus);
    chartView->setFrameShape(QFrame::NoFrame);
    chartView->setRubberBand(QChartView::NoRubberBand);
    chartView->setRenderHint(QPainter::Antialiasing, true);
    chartView->setBackgroundBrush(Qt::transparent);
    mainLayout->addWidget(chartView, 1);

    connect(metricToggle, &QPushButton::toggled, this, [this](bool checked) {
        showingLand = checked;
        refreshChart();
    });
    connect(scaleToggle, &QPushButton::toggled, this, [this](bool checked) {
        usingLogScale = checked;
        refreshChart();
    });

    seriesData.resize(static_cast<size_t>(playerCount));
    for (int i = 0; i < playerCount; ++i) {
        const QColor& color = playerColors[static_cast<size_t>(i)];
        const QString& name = playerNames[static_cast<size_t>(i)];

        QPen pen(color);
        pen.setWidth(2);

        QLineSeries* lineSeries = new QLineSeries(chart);
        lineSeries->setName(name);
        lineSeries->setPen(pen);
        chart->addSeries(lineSeries);
        lineSeries->attachAxis(axisX);
        lineSeries->attachAxis(axisYLinear);

        seriesData[static_cast<size_t>(i)].series = lineSeries;
    }

    refreshChart();
}

void AnalysisChartWidget::updateAnalysis(
    const std::vector<LeaderboardRow>& rows) {
    const qreal stepValue = static_cast<qreal>(sampleCount);
    army_t maxArmy = armyMax;
    int maxLand = landMax;

    std::vector<bool> updated(seriesData.size(), false);
    for (const LeaderboardRow& row : rows) {
        if (row.playerId < 0 ||
            static_cast<size_t>(row.playerId) >= seriesData.size()) {
            continue;
        }

        PlayerAnalysisSeries& series =
            seriesData[static_cast<size_t>(row.playerId)];
        appendAnalysisHistories(series, stepValue, static_cast<qreal>(row.army),
                                static_cast<qreal>(row.land));

        maxArmy = std::max(maxArmy, row.army);
        maxLand = std::max(maxLand, row.land);
        updated[static_cast<size_t>(row.playerId)] = true;
    }

    for (size_t i = 0; i < seriesData.size(); ++i) {
        if (updated[i]) {
            continue;
        }
        appendAnalysisHistories(seriesData[i], stepValue, 0.0, 0.0);
    }

    ++sampleCount;
    armyMax = maxArmy;
    landMax = maxLand;
    updateAxisRanges();

    for (PlayerAnalysisSeries& playerSeries : seriesData) {
        const QList<QPointF>& history =
            historyForMode(playerSeries, showingLand, usingLogScale);
        if (!history.isEmpty()) {
            playerSeries.series->append(history.constLast());
        }
    }
}

void AnalysisChartWidget::refreshChart() {
    const bool showLand = showingLand;
    const bool useLogScale = usingLogScale;
    chart->setTitle(
        QString("%1 Trend (%2)")
            .arg(showLand ? "Land" : "Army", useLogScale ? "Log" : "Linear"));

    metricToggle->setText(showLand ? "Switch to Army" : "Switch to Land");
    scaleToggle->setText(useLogScale ? "Switch to Linear" : "Switch to Log");

    QAbstractAxis* activeAxis = useLogScale
                                    ? static_cast<QAbstractAxis*>(axisYLog)
                                    : static_cast<QAbstractAxis*>(axisYLinear);
    QAbstractAxis* inactiveAxis = useLogScale
                                      ? static_cast<QAbstractAxis*>(axisYLinear)
                                      : static_cast<QAbstractAxis*>(axisYLog);

    for (PlayerAnalysisSeries& playerSeries : seriesData) {
        playerSeries.series->detachAxis(inactiveAxis);
    }

    if (chart->axes(Qt::Vertical).contains(inactiveAxis))
        chart->removeAxis(inactiveAxis);
    if (!chart->axes(Qt::Vertical).contains(activeAxis))
        chart->addAxis(activeAxis, Qt::AlignLeft);

    updateAxisRanges();

    for (PlayerAnalysisSeries& playerSeries : seriesData) {
        playerSeries.series->attachAxis(activeAxis);
        playerSeries.series->replace(
            historyForMode(playerSeries, showLand, useLogScale));
    }
}

void AnalysisChartWidget::updateAxisRanges() {
    const qreal axisMaxX =
        std::max<qreal>(1.0, static_cast<qreal>(std::max(0, sampleCount - 1)));
    axisX->setRange(0.0, axisMaxX);

    const double axisMaxY = showingLand
                                ? adjustedAxisMax(static_cast<double>(landMax))
                                : adjustedAxisMax(static_cast<double>(armyMax));
    if (usingLogScale)
        axisYLog->setRange(1.0, std::max(10.0, axisMaxY));
    else
        axisYLinear->setRange(0.0, axisMaxY);
}
