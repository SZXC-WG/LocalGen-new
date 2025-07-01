#include "mapWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QSvgRenderer>
#include <QWheelEvent>

MapWidget::MapWidget(QWidget* parent, int width, int height, bool focusEnabled)
    : QWidget(parent),
      scale(1.0),
      offset(0, 0),
      leftMouseDown(false),
      leftMouseDragging(false),
      rightMouseDown(false),
      rightMouseDragging(false),
      focusRow(-1),
      focusCol(-1),
      lastRightClickGrid(-1, -1) {
    setMouseTracking(true);
    setFocusEnabled(focusEnabled);
    displayTiles.resize(height, std::vector<DisplayTile>(width));
}

MapWidget::~MapWidget() {}

void MapWidget::setMapWidth(int w) {
    if (w != mapWidth()) {
        for (auto& row : displayTiles) {
            row.resize(w);
        }
        update();
    }
}

void MapWidget::setMapHeight(int h) {
    if (h != mapHeight()) {
        displayTiles.resize(h, std::vector<DisplayTile>(mapWidth()));
        update();
    }
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

    static QSvgRenderer renderer_city(QString(":/images/svg/city.svg")),
        renderer_general(QString(":/images/svg/crown.svg")),
        renderer_desert(QString(":/images/svg/desert.svg")),
        renderer_lookout(QString(":/images/svg/lookout.svg")),
        renderer_mountain(QString(":/images/svg/mountain.svg")),
        renderer_observatory(QString(":/images/svg/observatory.svg")),
        renderer_obstacle(QString(":/images/svg/obstacle.svg")),
        renderer_swamp(QString(":/images/svg/swamp.svg"));

    painter.setFont(QFont("Quicksand", 6));

    int h = mapHeight(), w = mapWidth();

    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            const DisplayTile& tile = displayTiles[r][c];
            QRectF cell(c * cellSize, r * cellSize, cellSize, cellSize);
            painter.setPen(QPen(Qt::black, 1));
            painter.fillRect(cell, tile.color);
            painter.drawRect(cell);
            if (tile.type != TILE_BLANK) {
                QRectF imgRect(c * cellSize + padding, r * cellSize + padding,
                               cellSize - padding * 2, cellSize - padding * 2);
                switch (tile.type) {
                    case TILE_CITY:
                        renderer_city.render(&painter, imgRect);
                        break;
                    case TILE_GENERAL:
                        renderer_general.render(&painter, imgRect);
                        break;
                    case TILE_DESERT:
                        renderer_desert.render(&painter, imgRect);
                        break;
                    case TILE_LOOKOUT:
                        renderer_lookout.render(&painter, imgRect);
                        break;
                    case TILE_MOUNTAIN:
                        renderer_mountain.render(&painter, imgRect);
                        break;
                    case TILE_OBSERVATORY:
                        renderer_observatory.render(&painter, imgRect);
                        break;
                    case TILE_OBSTACLE:
                        renderer_obstacle.render(&painter, imgRect);
                        break;
                    case TILE_SWAMP:
                        renderer_swamp.render(&painter, imgRect);
                        break;
                    default: break;
                }
            }
            if (!tile.text.isEmpty()) {
                painter.setPen(Qt::white);
                painter.drawText(cell, Qt::AlignCenter, tile.text);
            }
        }
    }

    if (focusRow != -1) {
        painter.setPen(QPen(Qt::white, 1.5));
        QRectF cell(focusCol * cellSize, focusRow * cellSize, cellSize,
                    cellSize);
        painter.drawRect(cell);
    }
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();
    QPoint globalMousePos = QCursor::pos();
    QPoint widgetMousePos = mapFromGlobal(globalMousePos);
    QPointF oldMousePos = (widgetMousePos - offset) / scale;
    if (delta > 0)
        scale *= zoomFactor;
    else
        scale /= zoomFactor;
    offset = widgetMousePos - oldMousePos * scale;
    update();
}

void MapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        leftMouseDown = true;
        lastMousePos = event->pos();
    } else if (event->button() == Qt::RightButton) {
        rightMouseDown = true;
        QPoint gridPos = mapToGrid(event->pos());
        if (isValidGridPos(gridPos)) {
            emit cellClicked(gridPos.y(), gridPos.x());
            lastRightClickGrid = gridPos;
        }
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    if (leftMouseDown) {
        if (!leftMouseDragging) {
            leftMouseDragging = true;
            setCursor(Qt::ClosedHandCursor);
        }
        QPoint delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
    } else if (rightMouseDown) {
        QPoint gridPos = mapToGrid(event->pos());
        if (isValidGridPos(gridPos) && gridPos != lastRightClickGrid) {
            emit cellClicked(gridPos.y(), gridPos.x());
            lastRightClickGrid = gridPos;
            rightMouseDragging = true;
        }
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        leftMouseDown = false;
        if (leftMouseDragging) {
            leftMouseDragging = false;
            setCursor(Qt::ArrowCursor);
        } else if (focusPolicy() == Qt::NoFocus) {
            // When focus is disabled, emit signal for left click
            QPoint gridPos = mapToGrid(event->pos());
            if (isValidGridPos(gridPos)) {
                emit cellClicked(gridPos.y(), gridPos.x());
            }
        } else {
            QPoint gridPos = mapToGrid(event->pos());
            if (isValidGridPos(gridPos)) {
                // important: transpose coordinates here
                focusRow = gridPos.y();
                focusCol = gridPos.x();
            } else {
                focusRow = focusCol = -1;
            }
            update();
        }
    } else if (event->button() == Qt::RightButton) {
        rightMouseDown = false;
        rightMouseDragging = false;
        lastRightClickGrid = QPoint(-1, -1);
    }
}

QPoint MapWidget::mapToGrid(const QPoint& pos) {
    QPointF scaledPos = (pos - offset) / scale;
    int gridX = static_cast<int>(scaledPos.x() / cellSize);
    int gridY = static_cast<int>(scaledPos.y() / cellSize);
    return QPoint(gridX, gridY);
}

void MapWidget::setFocusEnabled(bool enabled) {
    if (enabled) {
        setFocusPolicy(Qt::StrongFocus);
    } else {
        setFocusPolicy(Qt::NoFocus);
        focusRow = focusCol = -1;
        update();
    }
}

void MapWidget::keyPressEvent(QKeyEvent* event) {
    if (focusRow != -1 && focusCol != -1) {
        switch (event->key()) {
            case Qt::Key_Left:
                if (focusCol > 0) focusCol--;
                break;
            case Qt::Key_Right:
                if (focusCol < mapWidth() - 1) focusCol++;
                break;
            case Qt::Key_Up:
                if (focusRow > 0) focusRow--;
                break;
            case Qt::Key_Down:
                if (focusRow < mapHeight() - 1) focusRow++;
                break;
            default: QWidget::keyPressEvent(event); return;
        }
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}
