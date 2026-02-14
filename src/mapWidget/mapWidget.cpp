#include "mapWidget.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QWheelEvent>
#include <cmath>

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
    realloc(width, height);
}

MapWidget::~MapWidget() {}

void MapWidget::setMapWidth(int w) {
    if (w != mapWidth()) {
        for (auto& row : displayTiles) {
            row.resize(w);
        }
        repaint();
    }
}

void MapWidget::setMapHeight(int h) {
    if (h != mapHeight()) {
        displayTiles.resize(h, std::vector<DisplayTile>(mapWidth()));
        repaint();
    }
}

void MapWidget::realloc(int w, int h) {
    displayTiles.clear();
    displayTiles.resize(h, std::vector<DisplayTile>(w));
}

void MapWidget::fitCenter(int margin) {
    qreal mapPixelWidth = mapWidth() * cellSize;
    qreal mapPixelHeight = mapHeight() * cellSize;

    qreal availableWidth = width() - 2 * margin;
    qreal availableHeight = height() - 2 * margin;

    if (availableWidth <= 0 || availableHeight <= 0) {
        return;
    }

    qreal scaleX = availableWidth / mapPixelWidth;
    qreal scaleY = availableHeight / mapPixelHeight;
    scale = qMin(scaleX, scaleY);

    qreal scaledMapWidth = mapPixelWidth * scale;
    qreal scaledMapHeight = mapPixelHeight * scale;

    offset.setX((width() - scaledMapWidth) / 2.0);
    offset.setY((height() - scaledMapHeight) / 2.0);

    update();
}

static void drawArrow(QPainter& painter, const QPointF& p1, const QPointF& p2,
                      qreal length, qreal width) {
    // 1. Calculate geometry
    auto angle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());
    QPointF arrowEnd =
        p1 + QPointF(std::cos(angle) * length, std::sin(angle) * length);

    qreal arrowHeadLen = length / 3.0;
    QPointF left =
        arrowEnd - QPointF(std::cos(angle + M_PI / 4) * arrowHeadLen,
                           std::sin(angle + M_PI / 4) * arrowHeadLen);
    QPointF right =
        arrowEnd - QPointF(std::cos(angle - M_PI / 4) * arrowHeadLen,
                           std::sin(angle - M_PI / 4) * arrowHeadLen);

    QPainterPath path;
    path.moveTo(p1);
    path.lineTo(arrowEnd);
    path.moveTo(left);
    path.lineTo(arrowEnd);
    path.lineTo(right);

    // 2. Draw Outline
    QPen outlinePen(QColor(0, 0, 0, 80));
    outlinePen.setWidthF(width * 2.0);
    outlinePen.setCapStyle(Qt::RoundCap);
    outlinePen.setJoinStyle(Qt::RoundJoin);

    painter.setPen(outlinePen);
    painter.drawPath(path);

    // 3. Draw Main Body (Foreground)
    QPen mainPen(Qt::white);
    mainPen.setWidthF(width);
    mainPen.setCapStyle(Qt::RoundCap);
    mainPen.setJoinStyle(Qt::RoundJoin);

    painter.setPen(mainPen);
    painter.drawPath(path);
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
        renderer_swamp(QString(":/images/svg/swamp.svg")),
        renderer_light(QString(":/images/svg/light.svg"));

    painter.setFont(QFont("Quicksand", 6));

    int h = mapHeight(), w = mapWidth();

    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            const DisplayTile& tile = displayTiles[r][c];
            QRectF cell(c * cellSize, r * cellSize, cellSize, cellSize);
            painter.setPen(QPen(Qt::black, 1.0 / scale));
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
            if (tile.lightIcon) {
                QRectF lightRect =
                    tile.type == TILE_BLANK && tile.text.isEmpty()
                        ? QRectF(c * cellSize + padding, r * cellSize + padding,
                                 cellSize - padding * 2, cellSize - padding * 2)
                        : QRect(c * cellSize + 0.5 * padding,
                                r * cellSize + 0.5 * padding, 0.2 * cellSize,
                                0.2 * cellSize);
                renderer_light.render(&painter, lightRect);
            }
            if (!tile.text.isEmpty()) {
                painter.setPen(Qt::white);
                painter.drawText(cell, Qt::AlignCenter, tile.text);
            }
        }
    }

    if (focusRow != -1) {
        painter.setPen(QPen(Qt::white, 1.35 / scale));
        QRectF cell(focusCol * cellSize, focusRow * cellSize, cellSize,
                    cellSize);
        painter.drawRect(cell);
    }

    if (moveQueue == nullptr || moveQueue->empty()) return;

    for (const auto& move : *moveQueue) {
        if (move.type == MoveType::MOVE_ARMY) {
            int r1 = move.from.x - 1, c1 = move.from.y - 1;
            int r2 = move.to.x - 1, c2 = move.to.y - 1;

            QPointF p1((c1 + 0.5) * cellSize, (r1 + 0.5) * cellSize);
            QPointF p2((c2 + 0.5) * cellSize, (r2 + 0.5) * cellSize);

            drawArrow(painter, p1, p2, cellSize * 0.3, 1.5 / scale);
        }
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
        leftMousePressPos = lastMousePos = event->pos();
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
            static const int dragThreshold = QApplication::startDragDistance();
            if ((event->pos() - leftMousePressPos).manhattanLength() >=
                dragThreshold) {
                leftMouseDragging = true;
                setCursor(Qt::ClosedHandCursor);
                lastMousePos = event->pos();
            }
            return;
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

QPoint MapWidget::mapToGrid(const QPoint& pos) const {
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
    if (moveQueue) {
        if (event->key() == Qt::Key_Q) {
            moveQueue->clear();
            update();
            return;
        }
        if (event->key() == Qt::Key_E) {
            if (!moveQueue->empty()) {
                Coord fromTile = moveQueue->back().from;
                moveQueue->pop_back();
                focusRow = fromTile.x - 1;
                focusCol = fromTile.y - 1;
                update();
            }
            return;
        }
    }

    if (focusRow != -1 && focusCol != -1) {
        int oldRow = focusRow, oldCol = focusCol;
        switch (event->key()) {
            case Qt::Key_Left:
                if (focusCol == 0) return;
                focusCol--;
                break;
            case Qt::Key_Right:
                if (focusCol == mapWidth() - 1) return;
                focusCol++;
                break;
            case Qt::Key_Up:
                if (focusRow == 0) return;
                focusRow--;
                break;
            case Qt::Key_Down:
                if (focusRow == mapHeight() - 1) return;
                focusRow++;
                break;
            default: QWidget::keyPressEvent(event); return;
        }
        if (moveQueue) {
            // TODO: align with official keys (Z for 50%)
            bool takeHalf = (event->modifiers() & Qt::ControlModifier);
            moveQueue->emplace_back(
                MoveType::MOVE_ARMY, Coord(oldRow + 1, oldCol + 1),
                Coord(focusRow + 1, focusCol + 1), takeHalf);
        }
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void MapWidget::bindMoveQueue(std::deque<Move>* queue) {
    moveQueue = queue;
    update();
}
