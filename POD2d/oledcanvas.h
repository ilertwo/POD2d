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
#include <QQueue>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>

struct HistoryStep {
    int layerIndex;
    QImage previousState;
    QImage newState;
};

enum class DrawTool { Pen, Line, Rectangle, Circle, Fill, BrokenLine, Text, Copy, Cut, PasteShape, Pan };

class OledCanvas : public QWidget {
    Q_OBJECT

public:
    explicit OledCanvas(QWidget *parent = nullptr);

    void setTool(DrawTool tool);
    void copyLayer();
    void cutLayer();
    void pasteToLayer();

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

    DrawTool currentTool = DrawTool::Pen;
    QPoint startPoint;
    QPoint lastPoint;
    bool isDrawing = false;

    void floodFill(int x, int y, QColor targetColor, QColor replacementColor);

private:
    QImage canvasImage;
    double scaleFactor;
    QPointF offset;
    QImage tempState;
    QList<QImage> layers;
    int currentLayerIndex;
    QList<HistoryStep> history;
    int historyIndex = -1;
    QRect selectionRect;
    bool hasSelection = false;
    QImage pastedImage;
    QImage internalClipboard;
    QPoint lastPanPoint;
};

#endif // OLEDCANVAS_H
