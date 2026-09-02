#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H

#include <QWidget>
#include <QImage>
#include <QRect>
#include <QPoint>
#include <QPointF>
#include <QPainterPath>
#include "projectmodel.h"

// Enum
enum class DrawTool { Pen, Line, Rectangle, Circle, Fill, BrokenLine, Text, Select, Pan, Brush, Dithering, Eraser, Pipette, LassoSelect, ShapeSelect, Lighten};

enum class HandleType { None, TopLeft, TopRight, BottomLeft, BottomRight, Move };

class PixelCanvas : public QWidget {
    Q_OBJECT

public:
    explicit PixelCanvas(QWidget *parent = nullptr);

    // Interacting with the model
    void setModel(ProjectModel *model);

    // Tool settings
    void setTool(DrawTool tool);
    void setBrushSize(int size);
    void setZoom(double scale);
    void setCanvasSize(int width, int height);
    int getBrushSize() const;

    // Methods for setting colors
    void setPrimaryColor(const QColor &color);
    void setSecondaryColor(const QColor &color);
    QColor getPrimaryColor() const;
    QColor getSecondaryColor() const;
    void setShowGrid(bool show);
    void setGridColor(const QColor &color);
    void setBackgroundStyle(const QString &style);

    // Public actions (invoked from MainWindow, for example, via menus or hotkeys)
    void rotateFloatingImage();
    void commitFloatingImage();
    void copyLayer();
    void cutLayer();
    void pasteToLayer();
    void fitToScreen();
    void resetToolState();
    void setVerticalMirror(bool enabled);

signals:
    void colorPicked(const QColor &color, bool isPrimary);
    void cursorPositionChanged(int x, int y);

protected:
    // Qt Events
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // Internal helpers (hidden from other classes)
    HandleType getHandleAt(const QPoint& pos);
    void clampOffset();

    // State data
    ProjectModel *m_model = nullptr;
    DrawTool currentTool = DrawTool::Pen;

    QColor primaryColor = Qt::white;
    QColor secondaryColor = Qt::black;
    int canvasWidth = 128;
    int canvasHeight = 64;
    int brushSize = 1;
    double scaleFactor = 5.0;
    QPointF offset;
    QPoint currentMousePos;
    bool isMouseOnCanvas = false;

    bool m_showGrid = true;
    QColor m_gridColor = QColor(50, 50, 50);
    QString m_bgStyle = "Checkerboard";

    // Drawing supplies
    QPoint startPoint;
    QPoint lastPoint;
    bool isDrawing = false;
    QImage tempState;
    QPainterPath selectionPath;
    QPainterPath dragStartPath;
    bool m_verticalMirror = false;
    QPoint getMirroredPoint(const QPoint &pt) const;

    // Variables for highlighting
    QRect selectionRect;
    bool hasSelection = false;
    bool isFloating = false;
    QImage floatingImage;
    QImage originalFloatingImage;
    HandleType activeHandle = HandleType::None;
    QPoint dragStartMousePos;
    QRect dragStartRect;
    QPoint lastPanPoint;

};

#endif // PIXELCANVAS_H
