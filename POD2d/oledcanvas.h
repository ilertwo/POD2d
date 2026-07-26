#ifndef OLEDCANVAS_H
#define OLEDCANVAS_H

#include <QWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QGraphicsView>
#include <QWheelEvent>
#include <QScrollBar>

class OledCanvas : public QWidget {
    Q_OBJECT

public:
    explicit OledCanvas(QWidget *parent = nullptr);

    QString generateArduinoCode();
    void generateImage(QString code);
    void clearCanvas();
    void setZoom(double scale);
    void undo();
    void redo();

    void setCanvasImage(QImage image);

signals:
    void imageChanged(const QImage &newImage);

protected:
    void saveToHistory();
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void clampOffset();

private:
    QImage canvasImage;
    double scaleFactor;
    QPointF offset;
    QList<QImage> history;
    int historyIndex = -1;
};

#endif // OLEDCANVAS_H
