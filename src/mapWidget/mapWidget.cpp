#include "mapWidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSvgRenderer>

MapWidget::MapWidget(QWidget* parent, int w, int h, bool focusEnabled) :
    QWidget(parent), scale(1.0), offset(0, 0), mouseDown(false), isDragging(false), width(w), height(h), focusX(-1), focusY(-1) {
    setMouseTracking(true);
    setFocusEnabled(focusEnabled);
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

    const qreal padding = cellSize * paddingFactor;

    static const QColor bg(220, 220, 220);
    static const QColor playerColors[] = {
        QColor(255, 0, 0),
        QColor(255, 112, 16),
        QColor(0, 128, 0),
        QColor(16, 49, 255)
    };

    static QSvgRenderer
        renderer_city(QString(":/images/svg/city.svg")),
        renderer_general(QString(":/images/svg/crown.svg")),
        renderer_desert(QString(":/images/svg/desert.svg")),
        renderer_lookout(QString(":/images/svg/lookout.svg")),
        renderer_mountain(QString(":/images/svg/mountain.svg")),
        renderer_observatory(QString(":/images/svg/observatory.svg")),
        renderer_obstacle(QString(":/images/svg/obstacle.svg")),
        renderer_swamp(QString(":/images/svg/swamp.svg"));

    QSvgRenderer* renderers[] = {
        &renderer_city, &renderer_general, &renderer_desert, &renderer_lookout,
        &renderer_mountain, &renderer_observatory, &renderer_obstacle, &renderer_swamp
    };

    QRandomGenerator* rand = QRandomGenerator::global();

    painter.setPen(QPen(Qt::black, 1));
    for(int i = 0; i < width; ++i) {
        for(int j = 0; j < height; ++j) {
            QRectF cell(i * cellSize, j * cellSize, cellSize, cellSize);
            painter.fillRect(cell, rand->bounded(2) == 0 ? bg : playerColors[rand->bounded(4)]);
            painter.drawRect(cell);
            int k = rand->bounded(15);
            if(k < 8) {
                QRectF imgRect(i * cellSize + padding, j * cellSize + padding,
                               cellSize - padding * 2, cellSize - padding * 2);
                QSvgRenderer* renderer = renderers[k];
                renderer->render(&painter, imgRect);
            }
        }
    }

    if(focusX != -1) {
        painter.setPen(QPen(Qt::white, 1.5));
        QRectF cell(focusX * cellSize, focusY * cellSize, cellSize, cellSize);
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
        } else if(focusPolicy() != Qt::NoFocus) {
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
    int gridX = static_cast<int>(scaledPos.x() / cellSize);
    int gridY = static_cast<int>(scaledPos.y() / cellSize);
    return QPoint(gridX, gridY);
}

void MapWidget::setFocusEnabled(bool enabled) {
    if(enabled) {
        setFocusPolicy(Qt::StrongFocus);
    } else {
        setFocusPolicy(Qt::NoFocus);
        focusX = focusY = -1;
        update();
    }
}

void MapWidget::keyPressEvent(QKeyEvent* event) {
    if(focusX != -1 && focusY != -1) {
        switch(event->key()) {
            case Qt::Key_Left:
                if(focusX > 0) focusX--;
                break;
            case Qt::Key_Right:
                if(focusX < width - 1) focusX++;
                break;
            case Qt::Key_Up:
                if(focusY > 0) focusY--;
                break;
            case Qt::Key_Down:
                if(focusY < height - 1) focusY++;
                break;
            default:
                QWidget::keyPressEvent(event);
                return;
        }
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}
