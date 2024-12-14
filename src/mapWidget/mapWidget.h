#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QPointF>

class MapWidget : public QWidget {
    Q_OBJECT

   public:
    explicit MapWidget(QWidget* parent = nullptr);
    ~MapWidget();

   protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    float scale;
    QPointF offset;
};

#endif  // MAPWIDGET_H
