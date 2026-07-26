#include "wheelmouse.h"
#include <QScrollBar>
#include <QAbstractItemView>
#include <cmath>

//Конструктор
wheelMouse::wheelMouse(QWidget *parent) : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    setDragMode(QGraphicsView::ScrollHandDrag);
}

//Обробка прокрутки колеса миші або тачпада
void wheelMouse::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {

        const double scaleFactor = 1.15;
        const double minScale = 0.1;
        const double maxScale = 3.0;

        double currentScale = transform().m11();
        double newScale = currentScale;

        if (event->angleDelta().y() > 0) newScale *= scaleFactor;
        else newScale /= scaleFactor;

        if (newScale > maxScale) newScale = maxScale;
        if (newScale < minScale) newScale = minScale;

        double factor = newScale / currentScale;
        scale(factor, factor);

        emit scaleChanged(newScale);
        event->accept();
        return;
    }

    QPoint pixelDelta = event->pixelDelta();
    QPoint angleDelta = event->angleDelta();

    if (!pixelDelta.isNull()) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - pixelDelta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - pixelDelta.y());

    } else if (!angleDelta.isNull()) {

        int step = 20;

        if (angleDelta.x() != 0) {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - (angleDelta.x() > 0 ? step : -step));
        }
        if (angleDelta.y() != 0) {
            verticalScrollBar()->setValue(verticalScrollBar()->value() - (angleDelta.y() > 0 ? step : -step));
        }
    }

    event->accept();
}

//Слот для зміни масштабу ззовні
void wheelMouse::setZoom(double newScale)
{
    double currentScale = transform().m11();

    if (qAbs(newScale - currentScale) < 0.001) return;

    double factor = newScale / currentScale;
    scale(factor, factor);
}
