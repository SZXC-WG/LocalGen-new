#include "mapWidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QRandomGenerator>
#include <QMouseEvent>

MapWidget::MapWidget(QWidget* parent, int w, int h) :
    QWidget(parent), scale(1.0), offset(0, 0), mouseDown(false), isDragging(false), width(w), height(h), focusX(-1), focusY(-1) {
    setMouseTracking(true);
}

MapWidget::~MapWidget() {
}

void MapWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.translate(offset);
    painter.scale(scale, scale);

    const qreal
        cellWidth = defaultSize / width,
        cellHeight = defaultSize / height,
        paddingW = cellWidth * paddingFactor, paddingH = cellHeight * paddingFactor;

    static const QColor bg(220, 220, 220);
    static const QColor playerColors[] = {
        QColor(255, 0, 0),
        QColor(255, 112, 16),
        QColor(0, 128, 0),
        QColor(16, 49, 255)
    };

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

    QRandomGenerator* rand = QRandomGenerator::global();

    painter.setPen(QPen(Qt::black, 1));
    for(int i = 0; i < width; ++i) {
        for(int j = 0; j < height; ++j) {
            QRectF cell(i * cellWidth, j * cellHeight, cellWidth, cellHeight);
            painter.fillRect(cell, rand->bounded(2) == 0 ? bg : playerColors[rand->bounded(4)]);
            painter.drawRect(cell);
            int k = rand->bounded(15);
            if(k < 8) {
                QRectF imgRect(i * cellWidth + paddingW, j * cellHeight + paddingH,
                               cellWidth - paddingW * 2, cellHeight - paddingH * 2);
                const QPixmap& pixmap = pixmaps[k];
                painter.drawPixmap(imgRect, pixmap, pixmap.rect());
            }
        }
    }

    if(focusX != -1) {
        painter.setPen(QPen(Qt::white, 1.5));
        QRectF cell(focusX * cellWidth, focusY * cellHeight, cellWidth, cellHeight);
        painter.drawRect(cell);
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
        mouseDown = true;
        lastMousePos = event->pos();
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    if(mouseDown) {
        if(!isDragging) {
            isDragging = true;
            setCursor(Qt::ClosedHandCursor);
        }
        QPoint delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event) {
    if(event->button() == Qt::LeftButton) {
        mouseDown = false;
        if(isDragging) {
            isDragging = false;
            setCursor(Qt::ArrowCursor);
        } else {
            QPoint gridPos = mapToGrid(event->pos());
            if(gridPos.x() >= 0 && gridPos.x() < width && gridPos.y() >= 0 && gridPos.y() < height) {
                focusX = gridPos.x();
                focusY = gridPos.y();
            } else {
                focusX = focusY = -1;
            }
            update();
        }
    }
}

QPoint MapWidget::mapToGrid(const QPoint& pos) {
    QPointF scaledPos = (pos - offset) / scale;
    int gridX = static_cast<int>(scaledPos.x() / (defaultSize / width));
    int gridY = static_cast<int>(scaledPos.y() / (defaultSize / height));
    return QPoint(gridX, gridY);
}
