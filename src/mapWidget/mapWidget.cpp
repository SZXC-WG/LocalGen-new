#include "mapWidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QRandomGenerator>
#include <QMouseEvent>

MapWidget::MapWidget(QWidget* parent, int w, int h) :
    QWidget(parent), scale(1.0), offset(0, 0), isDragging(false), width(w), height(h) {
    setMouseTracking(true);
}

MapWidget::~MapWidget() {
}

void MapWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(offset);
    painter.scale(scale, scale);

    const qreal cellWidth = 300.0 / width, cellHeight = 300.0 / height;

    for(int i = 0; i < width; ++i) {
        for(int j = 0; j < height; ++j) {
            QRectF cell(i * cellWidth, j * cellHeight, cellWidth, cellHeight);
            painter.fillRect(cell, QColor(220, 220, 220));
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
