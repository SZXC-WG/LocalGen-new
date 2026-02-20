#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QPixmap>
#include <QPoint>
#include <QWidget>
#include <deque>
#include <vector>

#include "../GameEngine/board.hpp"
#include "../GameEngine/move.hpp"

struct DisplayTile {
    tile_type_e type = TILE_BLANK;
    QColor color = QColor(220, 220, 220);  // Default color for TILE_BLANK
    QString text;
    bool lightIcon = false;
    bool displayBorders = true;
};

class MapWidget : public QWidget {
    Q_OBJECT

   public:
    explicit MapWidget(QWidget* parent, int w, int h, bool focusEnabled = true,
                       int fitMargin = 25);
    ~MapWidget();

    void setFocusEnabled(bool enabled);
    inline DisplayTile& tileAt(int r, int c) { return displayTiles[r][c]; }

    inline int mapWidth() const { return displayTiles[0].size(); }
    inline int mapHeight() const { return displayTiles.size(); }

    void setMapWidth(int w);
    void setMapHeight(int h);
    void realloc(int w, int h);

    void fitCenter();

    void bindMoveQueue(std::deque<Move>* queue);
    void setFocusCell(int r, int c);

   signals:
    void cellClicked(int r, int c);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

   private:
    // Helper functions
    QPoint mapToGrid(const QPoint& pos) const;

    inline bool isValidGridPos(int x, int y) const {
        return x >= 0 && x < mapWidth() && y >= 0 && y < mapHeight();
    }
    inline bool isValidGridPos(const QPoint& pos) const {
        return isValidGridPos(pos.x(), pos.y());
    }

    QPixmap& getPixmapCache(int cacheId, int physicalSize);

    // Constants
    static constexpr qreal cellSize = 20.0, zoomFactor = 1.1,
                           paddingFactor = 0.109375;
    const int fitMargin;

    // State variables
    int focusRow, focusCol;
    bool takingHalfArmy = false;

    qreal scale;
    QPointF offset;
    bool leftMouseDown, leftMouseDragging;
    bool rightMouseDown, rightMouseDragging;
    QPoint leftMousePressPos, lastMousePos;
    QPoint lastRightClickGrid;

    std::vector<std::vector<DisplayTile>> displayTiles;
    std::deque<Move>* moveQueue = nullptr;

    // Cached rendering data
    QPixmap pixmapCache[11];  // 0-8: TILE_SPAWN to TILE_OBSTACLE;
                              // 9-10: light (full/small)
};

#endif  // MAPWIDGET_H
