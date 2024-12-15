#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QPoint>

class MapWidget : public QWidget {
    Q_OBJECT

   public:
    explicit MapWidget(QWidget* parent, int w, int h);
    ~MapWidget();
    const int width, height;

   protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

   private:
    qreal scale;
    QPointF offset;
    bool isDragging;
    QPoint lastMousePos;
};

#endif  // MAPWIDGET_H
