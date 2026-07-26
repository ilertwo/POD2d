#include "oledcanvas.h"

OledCanvas::OledCanvas(QWidget *parent) : QWidget(parent) {
    scaleFactor = 5;

    canvasImage = QImage(128, 64, QImage::Format_RGB32);
    canvasImage.fill(Qt::black);
}

void OledCanvas::setCanvasImage(QImage image)
{
    canvasImage = image;

    update();
    emit imageChanged(canvasImage);
}

void OledCanvas::clearCanvas() {
    canvasImage.fill(Qt::black);
    update();
}

void OledCanvas::mouseMoveEvent(QMouseEvent *event) {
    int x = (event->pos().x() - offset.x()) / scaleFactor;
    int y = (event->pos().y() - offset.y()) / scaleFactor;

    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
        if (event->buttons() & Qt::LeftButton) {
            canvasImage.setPixelColor(x, y, Qt::white);
        } else if (event->buttons() & Qt::RightButton) {
            canvasImage.setPixelColor(x, y, Qt::black);
        }
        update();
        emit imageChanged(canvasImage);
    }
}

void OledCanvas::mousePressEvent(QMouseEvent *event) {
    mouseMoveEvent(event);
    saveToHistory();
}

void OledCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.translate(offset);
    painter.scale(scaleFactor, scaleFactor);
    painter.drawImage(0, 0, canvasImage);

    QPen gridPen(QColor(50, 50, 50));
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);

    for (int i = 0; i <= 128; ++i) painter.drawLine(i, 0, i, 64);
    for (int i = 0; i <= 64; ++i) painter.drawLine(0, i, 128, i);
}

QString OledCanvas::generateArduinoCode() {
    QString code = "const unsigned char my_drawing[] PROGMEM = {\n  ";
    int byteCount = 0;

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x += 8) {
            uint8_t currentByte = 0;
            for (int b = 0; b < 8; b++) {
                if (canvasImage.pixelColor(x + b, y) == Qt::white) {
                    currentByte |= (1 << (7 - b));
                }
            }
            code += QString("0x%1, ").arg(currentByte, 2, 16, QChar('0'));
            byteCount++;
            if (byteCount % 16 == 0) {
                code += "\n  ";
            }
        }
    }
    code += "\n};";
    return code;
}

void OledCanvas::generateImage(QString code)
{

    code = code.mid(48);
    QStringList hexValues = code.split(",", Qt::SkipEmptyParts);
    int byteIndex = 0;

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x += 8) {

            if (byteIndex >= hexValues.size()) {
                break;
            }

            QString hexStr = hexValues[byteIndex].trimmed();
            bool ok;
            uint8_t currentByte = hexStr.toInt(&ok, 16);

            if (!ok) {
                currentByte = 0;
            }

            for (int b = 0; b < 8; b++) {
                bool isBitSet = (currentByte & (1 << (7 - b))) != 0;

                if (isBitSet) {
                    canvasImage.setPixelColor(x + b, y, Qt::white);
                } else {
                    canvasImage.setPixelColor(x + b, y, Qt::black);
                }
            }

            byteIndex++;
        }
    }
    update();
}

void OledCanvas::saveToHistory() {
    while (history.size() > historyIndex + 1) {
        history.removeLast();
    }

    history.append(canvasImage);
    historyIndex++;
}

void OledCanvas::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const double zoomStep = 1.15;
        double oldScale = scaleFactor;

        if (event->angleDelta().y() > 0) {
            scaleFactor *= zoomStep;
        } else {
            scaleFactor /= zoomStep;
        }
        scaleFactor = qBound(1.0, scaleFactor, 40.0);

        QPointF mousePos = event->position();
        QPointF worldPos = (mousePos - offset) / oldScale;
        offset = mousePos - worldPos * scaleFactor;

        clampOffset();
        update();
        event->accept();
    } else {
        QPoint delta = event->angleDelta();

        offset.setX(offset.x() + delta.x());
        offset.setY(offset.y() + delta.y());

        clampOffset();
        update();
        event->accept();
    }
}

void OledCanvas::setZoom(double newScale) {
    if (qAbs(newScale - scaleFactor) < 0.001) return;

    scaleFactor = qBound(1.0, newScale, 40.0);
    update();
}


void OledCanvas::clampOffset() {
    double currentWidth = 128 * scaleFactor;
    double currentHeight = 64 * scaleFactor;

    double marginX = currentWidth * 0.2;
    double marginY = currentHeight * 0.2;

    double minX = -currentWidth + marginX;
    double minY = -currentHeight + marginY;

    double maxX = width() - marginX;
    double maxY = height() - marginY;

    offset.setX(qBound(minX, offset.x(), maxX));
    offset.setY(qBound(minY, offset.y(), maxY));
}

void OledCanvas::undo()
{
    if (historyIndex > 0) {
        historyIndex--;
        setCanvasImage(history[historyIndex]);
    }
}

void OledCanvas::redo()
{
    if (historyIndex < history.size() - 1) {
        historyIndex++;
        setCanvasImage(history[historyIndex]);
    }
}
