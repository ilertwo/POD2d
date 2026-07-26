#ifndef WHEELMOUSE_H
#define WHEELMOUSE_H

#include <QGraphicsView>
#include <QWheelEvent>

class wheelMouse : public QGraphicsView
{
    Q_OBJECT

public:
    explicit wheelMouse(QWidget *parent = nullptr);

public slots:
    void setZoom(double scale);

signals:
    void scaleChanged(double newScale);

protected:
    void wheelEvent(QWheelEvent *event) override;
};

#endif
