#include "mapWidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QRandomGenerator>
#include <QMouseEvent>

MapWidget::MapWidget(QWidget* parent) :
    QWidget(parent), scale(1.0), offset(0, 0), isDragging(false) {
    // setFixedSize(800, 600);
    setMouseTracking(true);
}

MapWidget::~MapWidget() {
}

void MapWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(offset);
    painter.scale(scale, scale);

    const int gridSize = 20;
    const qreal cellSize = 300.0 / gridSize;

    for(int i = 0; i < gridSize; ++i) {
        for(int j = 0; j < gridSize; ++j) {
            QRectF cell(i * cellSize, j * cellSize, cellSize, cellSize);
            painter.drawRect(cell);

            int randomNumber = QRandomGenerator::global()->bounded(10);
            painter.drawText(cell, Qt::AlignCenter, QString::number(randomNumber));
        }
    }
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();
    qreal factor = (delta > 0) ? 1.1 : 0.9;
    QPoint globalMousePos = QCursor::pos();
    QPoint widgetMousePos = mapFromGlobal(globalMousePos);
    QPointF oldMousePos = (widgetMousePos - offset) / scale;
    scale *= factor;
    offset = widgetMousePos - oldMousePos * scale;
    update();
}

void MapWidget::mousePressEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    if(isDragging) {
        QPoint delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}
