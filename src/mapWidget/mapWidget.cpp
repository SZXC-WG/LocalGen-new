#include "mapWidget.h"
#include <QPainter>
#include <QWheelEvent>

MapWidget::MapWidget(QWidget* parent) :
    QWidget(parent), scale(1.0), offset(0, 0) {
    setFixedSize(800, 600);
}

MapWidget::~MapWidget() {
}

void MapWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.translate(offset);
    painter.scale(scale, scale);
    painter.setBrush(Qt::red);
    // demo rect
    painter.drawRect(50, 50, 200, 100);
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();
    float factor = (delta > 0) ? 1.1 : 0.9;
    QPoint globalMousePos = QCursor::pos();
    QPoint widgetMousePos = mapFromGlobal(globalMousePos);
    QPointF oldMousePos = (widgetMousePos - offset) / scale;
    scale *= factor;
    offset = widgetMousePos - oldMousePos * scale;
    update();
}
