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
#include <QDataStream>
#include <QTimer>

#include "projectmodel.h"

enum class DrawTool { Pen, Line, Rectangle, Circle, Fill, BrokenLine, Text, Select, Pan, Brush, Dithering };

class OledCanvas : public QWidget {
    Q_OBJECT
public:
    explicit OledCanvas(QWidget *parent = nullptr);

    void setModel(ProjectModel *model);

    void setTool(DrawTool tool);
    void setBrushSize(int size);
    void setZoom(double scale);
    int getBrushSize() const;

    HandleType getHandleAt(const QPoint& pos);
    void leaveEvent(QEvent *event);
    void rotateFloatingImage();
    void clampOffset();
    void commitFloatingImage();
    void copyLayer();
    void cutLayer();
    void pasteToLayer();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    ProjectModel *m_model = nullptr;
    DrawTool currentTool = DrawTool::Pen;
    int brushSize = 1;
    double scaleFactor = 5.0;
    QPointF offset;
    QPoint currentMousePos;
    bool isMouseOnCanvas = false;

    QPoint startPoint;
    QPoint lastPoint;
    bool isDrawing = false;
    QImage canvasImage;
    QImage tempState;
    int historyIndex = -1;
    QRect selectionRect;
    bool hasSelection = false;
    QImage pastedImage;
    QImage internalClipboard;
    QPoint lastPanPoint;
    QImage floatingImage;
    QImage originalFloatingImage;
    bool isFloating = false;
    HandleType activeHandle = HandleType::None;
    QPoint dragStartMousePos;
    QRect dragStartRect;

    void drawBrush(int centerX, int centerY, QColor color);
    void floodFillDithering(int x, int y, QColor target, QColor replacement);
};

#endif // OLEDCANVAS_H

