#ifndef GENERALSBUTTON_HPP
#define GENERALSBUTTON_HPP

#include <qmath.h>

#include <QAbstractButton>
#include <QEnterEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStaticText>

class GeneralsButton : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(QColor bgColor READ bgColor WRITE setBgColor)
    Q_PROPERTY(QPointF shadowOffset READ shadowOffset WRITE setShadowOffset)

   public:
    explicit GeneralsButton(QWidget* parent = nullptr)
        : QAbstractButton(parent) {
        setCursor(Qt::PointingHandCursor);

        m_bgColor = Qt::white;
        m_tealColor = QColor(0, 128, 128);
        m_shadowOffset = QPointF(2.0, 2.0);

        m_lastDpr = -1.0;

        auto* bgAnim = new QPropertyAnimation(this, "bgColor", this);
        bgAnim->setDuration(200);
        bgAnim->setEasingCurve(QEasingCurve::InOutQuad);
        bgAnim->setStartValue(QColor(255, 255, 255));
        bgAnim->setEndValue(QColor(187, 187, 187));

        auto* shadowAnim = new QPropertyAnimation(this, "shadowOffset", this);
        shadowAnim->setDuration(200);
        shadowAnim->setEasingCurve(QEasingCurve::InOutQuad);
        shadowAnim->setStartValue(QPointF(2.0, 2.0));
        shadowAnim->setEndValue(QPointF(3.0, 3.0));

        m_animGroup = new QParallelAnimationGroup(this);
        m_animGroup->addAnimation(bgAnim);
        m_animGroup->addAnimation(shadowAnim);

        m_staticText.setTextFormat(Qt::PlainText);
        m_staticText.setPerformanceHint(QStaticText::AggressiveCaching);
    }

    QColor bgColor() const { return m_bgColor; }
    void setBgColor(const QColor& c) {
        m_bgColor = c;
        update();
    }

    QPointF shadowOffset() const { return m_shadowOffset; }
    void setShadowOffset(const QPointF& o) {
        m_shadowOffset = o;
        update();
    }

   protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);

        const qreal maxShadow = 3.0;
        QRectF btnRect(0, 0, width() - maxShadow, height() - maxShadow);

        painter.fillRect(btnRect.translated(m_shadowOffset), m_tealColor);
        painter.fillRect(btnRect, m_bgColor);

        updateStaticTextCache();

        painter.setPen(m_tealColor);

        QSizeF textSize = m_staticText.size();
        QPointF textPos(
            btnRect.x() + (btnRect.width() - textSize.width()) * 0.5,
            btnRect.y() + (btnRect.height() - textSize.height()) * 0.5);

        painter.drawStaticText(textPos, m_staticText);
    }

    void enterEvent(QEnterEvent* event) override {
        QAbstractButton::enterEvent(event);
        m_animGroup->setDirection(QAbstractAnimation::Forward);
        if (m_animGroup->state() != QAbstractAnimation::Running) {
            m_animGroup->start();
        }
    }

    void leaveEvent(QEvent* event) override {
        QAbstractButton::leaveEvent(event);
        m_animGroup->setDirection(QAbstractAnimation::Backward);
        if (m_animGroup->state() != QAbstractAnimation::Running) {
            m_animGroup->start();
        }
    }

    bool event(QEvent* e) override {
        if (e->type() == QEvent::FontChange) {
            m_staticText.prepare(QTransform(), font());
            updateGeometry();
        }
        return QAbstractButton::event(e);
    }

    QSize sizeHint() const override {
        updateStaticTextCache();
        QSizeF textSize = m_staticText.size();
        return QSize(qCeil(textSize.width()) + 3, qCeil(textSize.height()) + 3);
    }

    QSize minimumSizeHint() const override { return sizeHint(); }

   private:
    QColor m_bgColor;
    QColor m_tealColor;
    QPointF m_shadowOffset;
    QParallelAnimationGroup* m_animGroup;
    mutable QStaticText m_staticText;
    mutable qreal m_lastDpr;

    void updateStaticTextCache() const {
        qreal currentDpr = devicePixelRatioF();
        bool textChanged = (m_staticText.text() != text());
        bool dprChanged = !qFuzzyCompare(m_lastDpr, currentDpr);

        if (textChanged || dprChanged) {
            m_lastDpr = currentDpr;
            m_staticText.setText(text());
            m_staticText.prepare(QTransform(), font());
        }
    }
};

#endif  // GENERALSBUTTON_HPP
