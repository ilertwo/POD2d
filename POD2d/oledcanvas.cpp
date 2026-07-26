#include "oledcanvas.h"

OledCanvas::OledCanvas(QWidget *parent) : QWidget(parent) {
    scaleFactor = 5.0;

    QImage backgroundLayer(128, 64, QImage::Format_ARGB32);
    backgroundLayer.fill(Qt::black);

    layers.append(backgroundLayer);
    currentLayerIndex = 0;

    setMinimumSize(128, 64);
    saveToHistory();
}

void OledCanvas::setCanvasImage(QImage image)
{
    canvasImage = image;

    update();
    emit imageChanged(getFlattenedImage());
}

void OledCanvas::mouseMoveEvent(QMouseEvent *event) {
    int x = (event->pos().x() - offset.x()) / scaleFactor;
    int y = (event->pos().y() - offset.y()) / scaleFactor;

    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
        if (event->buttons() & Qt::LeftButton) {
            layers[currentLayerIndex].setPixelColor(x, y, Qt::white);
        } else if (event->buttons() & Qt::RightButton) {
            if (currentLayerIndex == 0) {
                layers[currentLayerIndex].setPixelColor(x, y, Qt::black);
            } else {
                layers[currentLayerIndex].setPixelColor(x, y, Qt::transparent);
            }
        }
        update();
        emit imageChanged(getFlattenedImage());
    }
}

void OledCanvas::mousePressEvent(QMouseEvent *event) {
    tempState = layers[currentLayerIndex];
    mouseMoveEvent(event);
}

void OledCanvas::mouseReleaseEvent(QMouseEvent *event) {
    saveToHistory();
}

void OledCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.translate(offset);
    painter.scale(scaleFactor, scaleFactor);

    painter.fillRect(0, 0, 128, 64, Qt::black);

    for (int i = 0; i < layers.size(); ++i) {

        if (i < currentLayerIndex) {
            int distance = currentLayerIndex - i;

            double opacity = qMax(0.1, 1.0 - (distance * 0.3));

            painter.setOpacity(opacity);

        } else if (i == currentLayerIndex) {
            painter.setOpacity(1.0);

        } else {
            painter.setOpacity(0.3);
        }

        painter.drawImage(0, 0, layers[i]);
    }

    painter.setOpacity(1.0);

    QPen gridPen(QColor(50, 50, 50));
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);

    for (int i = 0; i <= 128; ++i) painter.drawLine(i, 0, i, 64);
    for (int i = 0; i <= 64; ++i) painter.drawLine(0, i, 128, i);
}

#include <QDataStream>

QByteArray OledCanvas::saveProjectData() {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << layers;

    return data;
}

bool OledCanvas::loadProjectData(const QByteArray &data) {
    QDataStream stream(data);
    QList<QImage> loadedLayers;

    stream >> loadedLayers;

    if (loadedLayers.isEmpty()) {
        return false;
    }

    layers = loadedLayers;
    currentLayerIndex = layers.size() - 1;

    history.clear();
    historyIndex = -1;
    saveToHistory();

    update();
    emit imageChanged(getFlattenedImage());

    return true;
}

QString OledCanvas::generateArduinoCode() {
    QString code = "const unsigned char my_drawing[] PROGMEM = {\n  ";
    int byteCount = 0;

    QImage flatImage = getFlattenedImage();

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x += 8) {
            uint8_t currentByte = 0;
            for (int b = 0; b < 8; b++) {
                if (flatImage.pixelColor(x + b, y) == Qt::white) {
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

void OledCanvas::saveToHistory() {
    while (history.size() > historyIndex + 1) {
        history.removeLast();
    }

    HistoryStep step;
    step.layerIndex = currentLayerIndex;
    step.previousState = tempState;
    step.newState = layers[currentLayerIndex];

    history.append(step);
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

void OledCanvas::undo() {
    if (historyIndex >= 0) {
        HistoryStep step = history[historyIndex];

        layers[step.layerIndex] = step.previousState;

        historyIndex--;
        update();
        emit imageChanged(getFlattenedImage());
    }
}

void OledCanvas::redo() {
    if (historyIndex < history.size() - 1) {
        historyIndex++;

        HistoryStep step = history[historyIndex];

        layers[step.layerIndex] = step.newState;

        update();
        emit imageChanged(getFlattenedImage());
    }
}

void OledCanvas::addLayer() {
    QImage newLayer(128, 64, QImage::Format_ARGB32);
    newLayer.fill(Qt::transparent);

    layers.append(newLayer);

    currentLayerIndex = layers.size() - 1;

    update();
    saveToHistory();
}

void OledCanvas::setCurrentLayer(int index) {
    if (index >= 0 && index < layers.size()) {
        currentLayerIndex = index;
    }
    update();
}

void OledCanvas::clearCanvas() {
    tempState = layers[currentLayerIndex];

    if (currentLayerIndex == 0) {
        layers[currentLayerIndex].fill(Qt::black);
    } else {
        layers[currentLayerIndex].fill(Qt::transparent);
    }

    saveToHistory();

    update();
    emit imageChanged(getFlattenedImage());
}

int OledCanvas::getLayerCount(){
    return layers.size();
}

QImage OledCanvas::getFlattenedImage() {
    QImage flatImage(128, 64, QImage::Format_ARGB32);
    flatImage.fill(Qt::black);

    QPainter painter(&flatImage);
    for (int i = 0; i < layers.size(); ++i) {
        painter.drawImage(0, 0, layers[i]);
    }
    painter.end();

    return flatImage;
}
