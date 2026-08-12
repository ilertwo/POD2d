#include "painttools.h"

void PaintTools::drawBrush(QImage &image, int centerX, int centerY, int brushSize, const QColor &color) {
    int radius = brushSize / 2;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= radius * radius) {
                int px = centerX + dx;
                int py = centerY + dy;
                if (px >= 0 && px < image.width() && py >= 0 && py < image.height()) {
                    image.setPixelColor(px, py, color);
                }
            }
        }
    }
}

void PaintTools::floodFill(QImage &image, int x, int y, const QColor &targetColor, const QColor &replacementColor) {
    if (x < 0 || x >= image.width() || y < 0 || y >= image.height()) return;
    if (targetColor == replacementColor) return;
    if (image.pixelColor(x, y) != targetColor) return;

    QQueue<QPoint> queue;
    queue.enqueue(QPoint(x, y));

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();

        if (p.x() < 0 || p.x() >= image.width() || p.y() < 0 || p.y() >= image.height()) continue;
        if (image.pixelColor(p) != targetColor) continue;

        image.setPixelColor(p, replacementColor);

        queue.enqueue(QPoint(p.x() + 1, p.y()));
        queue.enqueue(QPoint(p.x() - 1, p.y()));
        queue.enqueue(QPoint(p.x(), p.y() + 1));
        queue.enqueue(QPoint(p.x(), p.y() - 1));
    }
}

void PaintTools::floodFillDithering(QImage &image, int startX, int startY, const QColor &targetColor, const QColor &replacementColor) {
    if (startX < 0 || startX >= image.width() || startY < 0 || startY >= image.height()) return;
    if (targetColor == replacementColor) return;
    if (image.pixelColor(startX, startY) != targetColor) return;

    QQueue<QPoint> queue;
    queue.enqueue(QPoint(startX, startY));

    QVector<QVector<bool>> visited(image.width(), QVector<bool>(image.height(), false));
    visited[startX][startY] = true;

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
            if (nx >= 0 && nx < image.width() && ny >= 0 && ny < image.height() && !visited[nx][ny]) {
                if (image.pixelColor(nx, ny) == targetColor) {
                    visited[nx][ny] = true;
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
    p.drawRect(QRect(start, end));
}

void PaintTools::drawCircle(QImage &image, const QPoint &start, const QPoint &end, const QColor &color) {
    QPainter p(&image);
    p.setPen(QPen(color, 1));
    p.setRenderHint(QPainter::Antialiasing, false);
    int radius = qMax(qAbs(start.x() - end.x()), qAbs(start.y() - end.y()));
    p.drawEllipse(start, radius, radius);
}
