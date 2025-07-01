#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QPoint>
#include <QWidget>
#include <vector>

#include "../GameEngine/board.h"

struct DisplayTile {
    tile_type_e type = TILE_BLANK;
    QColor color = Qt::white;
    QString text;
};

class MapWidget : public QWidget {
    Q_OBJECT

   public:
    explicit MapWidget(QWidget* parent, int w, int h, bool focusEnabled = true);
    ~MapWidget();

    void setFocusEnabled(bool enabled);
    DisplayTile& tileAt(int r, int c) { return displayTiles[r][c]; }

    int mapWidth() const { return displayTiles[0].size(); }
    int mapHeight() const { return displayTiles.size(); }

    void setMapWidth(int w);
    void setMapHeight(int h);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

   private:
    static constexpr qreal cellSize = 20.0, zoomFactor = 1.1,
                           paddingFactor = 0.109375;

    int focusRow, focusCol;

    QPoint mapToGrid(const QPoint& pos);

    qreal scale;
    QPointF offset;
    bool mouseDown, isDragging;
    QPoint lastMousePos;

    std::vector<std::vector<DisplayTile>> displayTiles;
};

#endif  // MAPWIDGET_H
