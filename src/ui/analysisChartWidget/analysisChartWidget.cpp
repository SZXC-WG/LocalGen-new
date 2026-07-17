// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#include "analysisChartWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QtCharts/QLegend>
#include <algorithm>
#include <cmath>

namespace {

double adjustedAxisMax(double value) {
    return value <= 1.0 ? 1.0 : value + std::max(1.0, value * 0.08);
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

QWidget* createHorizontalSwitch(const QString& leftText,
                                const QString& rightText,
                                const QString& accessibleName, QWidget* parent,
                                QSlider*& outSwitch) {
    QWidget* container = new QWidget(parent);
    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto createLabel = [container](const QString& text) {
        QLabel* label = new QLabel(text, container);
        label->setFont(QFont("Quicksand", 9, QFont::DemiBold));
        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, QColor(240, 244, 248));
        label->setPalette(palette);
        return label;
    };

    layout->addWidget(createLabel(leftText));

    outSwitch = new QSlider(Qt::Horizontal, container);
    outSwitch->setRange(0, 1);
    outSwitch->setPageStep(1);
    outSwitch->setFixedWidth(48);
    outSwitch->setAccessibleName(accessibleName);
    layout->addWidget(outSwitch);

    layout->addWidget(createLabel(rightText));
    return container;
}

qreal smoothedLinearValue(const QList<QPointF>& history, qreal rawValue) {
    constexpr qreal smoothingAlpha = 0.65;
    if (history.isEmpty() || rawValue <= 0.0) {
        return rawValue;
    }
    qreal lastValue = history.constLast().y();
    return lastValue + smoothingAlpha * (rawValue - lastValue);
}

qreal smoothedLogValue(const QList<QPointF>& history, qreal rawValue) {
    constexpr qreal smoothingAlpha = 0.65;
    if (rawValue < 1.0) return 1.0;
    if (history.isEmpty()) return rawValue;
    const qreal rawLogValue = std::log10(rawValue);
    const qreal previousSmoothedLog =
        std::log10(std::max<qreal>(1.0, history.constLast().y()));
    return std::pow(static_cast<qreal>(10.0),
                    previousSmoothedLog +
                        smoothingAlpha * (rawLogValue - previousSmoothedLog));
}

void appendAnalysisHistories(PlayerAnalysisSeries& series, qreal stepValue,
                             qreal armyValue, qreal landValue) {
    series.linearArmyHistory.append(QPointF(
        stepValue, smoothedLinearValue(series.linearArmyHistory, armyValue)));
    series.linearLandHistory.append(QPointF(
        stepValue, smoothedLinearValue(series.linearLandHistory, landValue)));
    series.logArmyHistory.append(
        QPointF(stepValue, smoothedLogValue(series.logArmyHistory, armyValue)));
    series.logLandHistory.append(
        QPointF(stepValue, smoothedLogValue(series.logLandHistory, landValue)));
}

const QList<QPointF>& historyForMode(const PlayerAnalysisSeries& series,
                                     bool showLand, bool useLogScale) {
    if (useLogScale) {
        return showLand ? series.logLandHistory : series.logArmyHistory;
    }
    return showLand ? series.linearLandHistory : series.linearArmyHistory;
}

QChart* createChart(const QString& title, QValueAxis*& outAxisX) {
    const QColor foreground(240, 244, 248);
    const QColor gridColor(240, 244, 248, 64);

    QChart* chart = new QChart();
    chart->setTitle(title);
    chart->setTitleFont(QFont("Quicksand", 11, QFont::Bold));
    chart->setTitleBrush(QBrush(foreground));
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(10, 10, 10, 10));
    chart->legend()->hide();

    outAxisX = new QValueAxis(chart);
    outAxisX->setTitleText("Step");
    outAxisX->setLabelFormat("%.0f");
    outAxisX->setTickCount(6);
    outAxisX->setRange(0.0, 1.0);
    styleAnalysisAxis(outAxisX, foreground, gridColor);
    chart->addAxis(outAxisX, Qt::AlignBottom);

    return chart;
}

}  // namespace

AnalysisChartWidget::AnalysisChartWidget(
    QWidget* parent, size_t playerCount,
    const std::vector<QColor>& playerColors,
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

    controlsLayout->addWidget(createHorizontalSwitch(
        "Army", "Land", "Chart metric: Army or Land", this, metricSwitch));

    controlsLayout->addStretch(1);
    controlsLayout->addWidget(createHorizontalSwitch(
        "Linear", "Log", "Chart scale: Linear or Log", this, scaleSwitch));
    mainLayout->addLayout(controlsLayout);

    chart = createChart("Army Trend", axisX);

    axisYLinear = new QValueAxis(chart);
    axisYLinear->setLabelFormat("%.0f");
    axisYLinear->setTickCount(6);
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
    chartView->setBackgroundBrush(QBrush(Qt::NoBrush));
    chartView->setAutoFillBackground(false);
    chartView->viewport()->setAutoFillBackground(false);
    chartView->viewport()->setAttribute(Qt::WA_TranslucentBackground, true);
    mainLayout->addWidget(chartView, 1);

    connect(metricSwitch, &QSlider::valueChanged, this, [this](int value) {
        showingLand = value != 0;
        refreshChart();
    });
    connect(scaleSwitch, &QSlider::valueChanged, this, [this](int value) {
        usingLogScale = value != 0;
        refreshChart();
    });

    seriesData.resize(playerCount);
    for (size_t i = 0; i < playerCount; ++i) {
        QPen pen(playerColors[i]);
        pen.setWidth(2);

        QLineSeries* lineSeries = new QLineSeries(chart);
        lineSeries->setName(playerNames[i]);
        lineSeries->setPen(pen);
        chart->addSeries(lineSeries);
        lineSeries->attachAxis(axisX);

        seriesData[i].series = lineSeries;
    }

    refreshChart();
}

void AnalysisChartWidget::updateAnalysis(const std::vector<RankItem>& rank) {
    const qreal stepValue = static_cast<qreal>(sampleCount);
    army_t maxArmy = armyMax;
    int maxLand = landMax;

    std::vector<bool> updated(seriesData.size(), false);
    for (const RankItem& item : rank) {
        size_t player = static_cast<size_t>(item.player);
        appendAnalysisHistories(seriesData[player], stepValue,
                                static_cast<qreal>(item.army),
                                static_cast<qreal>(item.land));

        maxArmy = std::max(maxArmy, item.army);
        maxLand = std::max(maxLand, item.land);
        updated[player] = true;
    }

    for (size_t i = 0; i < seriesData.size(); ++i) {
        if (!updated[i]) {
            appendAnalysisHistories(seriesData[i], stepValue, 0.0, 0.0);
        }
    }

    ++sampleCount;
    armyMax = maxArmy;
    landMax = maxLand;
    updateAxisRanges();

    for (PlayerAnalysisSeries& playerSeries : seriesData) {
        const QList<QPointF>& history =
            historyForMode(playerSeries, showingLand, usingLogScale);
        playerSeries.series->append(history.constLast());
    }
}

void AnalysisChartWidget::refreshChart() {
    chart->setTitle(QString("%1 Trend (%2)")
                        .arg(showingLand ? "Land" : "Army",
                             usingLogScale ? "Log" : "Linear"));

    QAbstractAxis* activeAxis =
        usingLogScale ? static_cast<QAbstractAxis*>(axisYLog) : axisYLinear;
    QAbstractAxis* inactiveAxis =
        usingLogScale ? static_cast<QAbstractAxis*>(axisYLinear) : axisYLog;

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
            historyForMode(playerSeries, showingLand, usingLogScale));
    }
}

void AnalysisChartWidget::updateAxisRanges() {
    const qreal axisMaxX = std::max(1, sampleCount - 1);
    axisX->setRange(0.0, axisMaxX);

    const double axisMaxY =
        adjustedAxisMax(static_cast<double>(showingLand ? landMax : armyMax));
    if (usingLogScale)
        axisYLog->setRange(1.0, std::max(10.0, axisMaxY));
    else
        axisYLinear->setRange(0.0, axisMaxY);
}
