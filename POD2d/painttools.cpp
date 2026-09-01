#include "painttools.h"
#include <QPainter>
#include <QQueue>
#include <QVector>

void PaintTools::drawBrush(QImage &image, int centerX, int centerY, int brushSize, const QColor &color) {
    QPainter p(&image);

    p.setCompositionMode(QPainter::CompositionMode_Source);

    p.setPen(QPen(color, brushSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPoint(centerX, centerY);
}

void PaintTools::floodFill(QImage &image, int x, int y, const QColor &targetColor, const QColor &replacementColor) {
    if (x < 0 || x >= image.width() || y < 0 || y >= image.height()) return;
    if (targetColor == replacementColor) return;
    if (image.pixelColor(x, y) != targetColor) return;

    QQueue<QPoint> queue;
    queue.enqueue(QPoint(x, y));

    image.setPixelColor(x, y, replacementColor);

    const int dx[] = {1, -1, 0, 0};
    const int dy[] = {0, 0, 1, -1};

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();

        for (int i = 0; i < 4; ++i) {
            int nx = p.x() + dx[i];
            int ny = p.y() + dy[i];

            if (nx >= 0 && nx < image.width() && ny >= 0 && ny < image.height()) {
                if (image.pixelColor(nx, ny) == targetColor) {
                    image.setPixelColor(nx, ny, replacementColor);
                    queue.enqueue(QPoint(nx, ny));
                }
            }
        }
    }
}

void PaintTools::floodFillDithering(QImage &image, int startX, int startY, const QColor &targetColor, const QColor &replacementColor) {
    const int width = image.width();
    const int height = image.height();

    if (startX < 0 || startX >= width || startY < 0 || startY >= height) return;
    if (targetColor == replacementColor) return;
    if (image.pixelColor(startX, startY) != targetColor) return;

    QVector<bool> visited(width * height, false);

    QQueue<QPoint> queue;
    queue.enqueue(QPoint(startX, startY));
    visited[startY * width + startX] = true;

    const int dx[] = {1, -1, 0, 0};
    const int dy[] = {0, 0, 1, -1};

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();
        int x = p.x();
        int y = p.y();

        if ((x + y) % 2 == 0) {
            image.setPixelColor(x, y, replacementColor);
        }

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int index = ny * width + nx;

                if (!visited[index] && image.pixelColor(nx, ny) == targetColor) {
                    visited[index] = true;
                    queue.enqueue(QPoint(nx, ny));
                }
            }
        }
    }
}

void PaintTools::drawLine(QImage &image, const QPoint &start, const QPoint &end, const QColor &color) {
    QPainter p(&image);
    p.setPen(QPen(color, 1));
    p.setRenderHint(QPainter::Antialiasing, false);
    p.drawLine(start, end);
}

void PaintTools::drawRect(QImage &image, const QPoint &start, const QPoint &end, const QColor &color) {
    QPainter p(&image);
    p.setPen(QPen(color, 1));
    p.setRenderHint(QPainter::Antialiasing, false);
    p.drawRect(QRect(start, end).normalized());
}

void PaintTools::drawCircle(QImage &image, const QPoint &start, const QPoint &end, const QColor &color) {
    QPainter p(&image);
    p.setPen(QPen(color, 1));
    p.setRenderHint(QPainter::Antialiasing, false);

    int dx = start.x() - end.x();
    int dy = start.y() - end.y();
    int radius = std::sqrt(dx * dx + dy * dy);

    p.drawEllipse(start, radius, radius);
}

void PaintTools::lightenBrush(QImage &image, int centerX, int centerY, int brushSize, bool darken) {
    int r = qMax(1, brushSize / 2);
    int rSquared = r * r;

    for (int y = centerY - r; y <= centerY + r; ++y) {
        for (int x = centerX - r; x <= centerX + r; ++x) {

            int dx = x - centerX;
            int dy = y - centerY;

            if (dx * dx + dy * dy <= rSquared) {
                if (x >= 0 && x < image.width() && y >= 0 && y < image.height()) {
                    QColor c = image.pixelColor(x, y);
                    if (c.alpha() > 0) {
                        image.setPixelColor(x, y, darken ? c.darker(110) : c.lighter(110));
                    }
                }
            }

        }
    }
}
