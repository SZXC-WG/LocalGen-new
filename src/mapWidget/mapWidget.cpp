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

    const qreal
        cellWidth = defaultSize / width,
        cellHeight = defaultSize / height,
        paddingW = cellWidth * paddingFactor, paddingH = cellHeight * paddingFactor;

    static QPixmap
        pixmap_city(":/images/img/city.png"),
        pixmap_general(":/images/img/crown.png"),
        pixmap_desert(":/images/img/desert.png"),
        pixmap_lookout(":/images/img/lookout.png"),
        pixmap_mountain(":/images/img/mountain.png"),
        pixmap_observatory(":/images/img/observatory.png"),
        pixmap_obstacle(":/images/img/obstacle.png"),
        pixmap_swamp(":/images/img/swamp.png");

    const QPixmap pixmaps[] = { pixmap_city, pixmap_general, pixmap_desert, pixmap_lookout,
                                pixmap_mountain, pixmap_observatory, pixmap_obstacle, pixmap_swamp };

    for(int i = 0; i < width; ++i) {
        for(int j = 0; j < height; ++j) {
            QRectF cell(i * cellWidth, j * cellHeight, cellWidth, cellHeight);
            painter.fillRect(cell, QColor(220, 220, 220));
            painter.drawRect(cell);

            int k = QRandomGenerator::global()->bounded(15);
            if(k < 8) {
                QRectF imgRect(i * cellWidth + paddingW, j * cellHeight + paddingH,
                               cellWidth - paddingW * 2, cellHeight - paddingH * 2);
                const QPixmap& pixmap = pixmaps[k];
                painter.drawPixmap(imgRect, pixmap, pixmap.rect());
            }
        }
    }
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();
    QPoint globalMousePos = QCursor::pos();
    QPoint widgetMousePos = mapFromGlobal(globalMousePos);
    QPointF oldMousePos = (widgetMousePos - offset) / scale;
    if(delta > 0) scale *= zoomFactor;
    else scale /= zoomFactor;
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
