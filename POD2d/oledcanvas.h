#ifndef OLEDCANVAS_H
#define OLEDCANVAS_H

#include <QWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

class OledCanvas : public QWidget {
    Q_OBJECT

public:
    explicit OledCanvas(QWidget *parent = nullptr);

    QString generateArduinoCode();
    void generateImage(QString code);
    void clearCanvas();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QImage canvasImage;
    int scaleFactor;
};

#endif // OLEDCANVAS_H
