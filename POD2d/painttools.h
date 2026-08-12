#ifndef PAINTTOOLS_H
#define PAINTTOOLS_H

#include <QImage>
#include <QColor>
#include <QPoint>
#include <QQueue>
#include <QPainter>

class PaintTools {
public:
    PaintTools() = delete;

    static void drawBrush(QImage &image, int centerX, int centerY, int brushSize, const QColor &color);
    static void floodFill(QImage &image, int x, int y, const QColor &targetColor, const QColor &replacementColor);
    static void floodFillDithering(QImage &image, int startX, int startY, const QColor &targetColor, const QColor &replacementColor);

    static void drawLine(QImage &image, const QPoint &start, const QPoint &end, const QColor &color);
    static void drawRect(QImage &image, const QPoint &start, const QPoint &end, const QColor &color);
    static void drawCircle(QImage &image, const QPoint &start, const QPoint &end, const QColor &color);
};

#endif // PAINTTOOLS_H
