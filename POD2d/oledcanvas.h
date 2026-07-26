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

struct HistoryStep {
    int layerIndex;
    QImage previousState;
    QImage newState;
};

class OledCanvas : public QWidget {
    Q_OBJECT

public:
    explicit OledCanvas(QWidget *parent = nullptr);

    bool loadProjectData(const QByteArray &data);
    QByteArray saveProjectData();

    QString generateArduinoCode();
    void clearCanvas();
    void setZoom(double scale);
    void undo();
    void redo();
    void addLayer();
    void setCurrentLayer(int index);
    int getLayerCount();

    void setCanvasImage(QImage image);

signals:
    void imageChanged(const QImage &newImage);

protected:
    QImage getFlattenedImage();
    void saveToHistory();
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void clampOffset();

private:
    QImage canvasImage;
    double scaleFactor;
    QPointF offset;
    QImage tempState;
    QList<QImage> layers;
    int currentLayerIndex;
    QList<HistoryStep> history;
    int historyIndex = -1;
};

#endif // OLEDCANVAS_H
