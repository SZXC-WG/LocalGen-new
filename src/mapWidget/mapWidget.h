#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QPoint>

class MapWidget : public QWidget {
    Q_OBJECT

   public:
    explicit MapWidget(QWidget* parent, int w, int h, bool focusEnabled = true);
    ~MapWidget();
    const int width, height;
    void setFocusEnabled(bool enabled);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

   private:
    static constexpr qreal
        cellSize = 20.0,
        zoomFactor = 1.1,
        paddingFactor = 0.109375;

    int focusX, focusY;

    QPoint mapToGrid(const QPoint& pos);

    qreal scale;
    QPointF offset;
    bool mouseDown, isDragging;
    QPoint lastMousePos;
};

#endif  // MAPWIDGET_H
