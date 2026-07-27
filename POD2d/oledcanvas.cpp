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
    if (!isDrawing) return;

    if (currentTool == DrawTool::Pan) {
        QPoint delta = event->pos() - lastPanPoint;

        offset.setX(offset.x() + delta.x());
        offset.setY(offset.y() + delta.y());

        clampOffset();

        lastPanPoint = event->pos();
        update();
        return;
    }

    int x = (event->pos().x() - offset.x()) / scaleFactor;
    int y = (event->pos().y() - offset.y()) / scaleFactor;

    if (currentTool == DrawTool::Select && activeHandle != HandleType::None) {
        QPoint currentPos(x, y);

        if (activeHandle == HandleType::Move) {
            QPoint delta = currentPos - dragStartMousePos;
            selectionRect = dragStartRect.translated(delta);
        } else {
            QRect newRect = dragStartRect;
            int dx = currentPos.x() - dragStartMousePos.x();
            int dy = currentPos.y() - dragStartMousePos.y();

            if (activeHandle == HandleType::TopLeft) newRect.setTopLeft(dragStartRect.topLeft() + QPoint(dx, dy));
            else if (activeHandle == HandleType::TopRight) newRect.setTopRight(dragStartRect.topRight() + QPoint(dx, dy));
            else if (activeHandle == HandleType::BottomLeft) newRect.setBottomLeft(dragStartRect.bottomLeft() + QPoint(dx, dy));
            else if (activeHandle == HandleType::BottomRight) newRect.setBottomRight(dragStartRect.bottomRight() + QPoint(dx, dy));

            bool flipH = newRect.width() < 0;
            bool flipV = newRect.height() < 0;

            selectionRect = newRect.normalized();

            if (isFloating) {
                int newW = qMax(1, selectionRect.width());
                int newH = qMax(1, selectionRect.height());

                QImage scaled = originalFloatingImage.scaled(newW, newH, Qt::IgnoreAspectRatio, Qt::FastTransformation);

                if (flipH || flipV) {
                    scaled = scaled.mirrored(flipH, flipV);
                }
                floatingImage = scaled;
            }
        }
        update();
        return;
    }

    x = qBound(-10, x, 138);
    y = qBound(-10, y, 74);
    QPoint currentPoint(x, y);

    QColor drawColor = (event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent;
    if (currentLayerIndex == 0 && drawColor == Qt::transparent) drawColor = Qt::black;

    if (currentTool == DrawTool::Pen) {
        if (x >= 0 && x < 128 && y >= 0 && y < 64) {
            layers[currentLayerIndex].setPixelColor(x, y, drawColor);
        }
    }
    else if (currentTool == DrawTool::Line || currentTool == DrawTool::Rectangle || currentTool == DrawTool::Circle) {
        layers[currentLayerIndex] = tempState;

        QPainter p(&layers[currentLayerIndex]);
        p.setPen(QPen(drawColor, 1));
        p.setRenderHint(QPainter::Antialiasing, false);

        if (currentTool == DrawTool::Line) {
            p.drawLine(startPoint, currentPoint);
        } else if (currentTool == DrawTool::Rectangle) {
            p.drawRect(QRect(startPoint, currentPoint));
        } else if (currentTool == DrawTool::Circle) {
            int radius = qMax(qAbs(startPoint.x() - x), qAbs(startPoint.y() - y));
            p.drawEllipse(startPoint, radius, radius);
        }
    }

    update();
    emit imageChanged(getFlattenedImage());
}

void OledCanvas::mousePressEvent(QMouseEvent *event) {

    if (currentTool == DrawTool::Pan) {
        lastPanPoint = event->pos();
        isDrawing = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    int x = (event->pos().x() - offset.x()) / scaleFactor;
    int y = (event->pos().y() - offset.y()) / scaleFactor;
    QPoint currentPos(x, y);

    if (currentTool == DrawTool::Select) {
        activeHandle = getHandleAt(currentPos);

        if (activeHandle != HandleType::None) {
            dragStartMousePos = currentPos;
            dragStartRect = selectionRect;
            isDrawing = true;
        } else {
            if (isFloating) {
                commitFloatingImage();
            } else {
                hasSelection = true;
                selectionRect = QRect(currentPos, QSize(0,0));
                dragStartMousePos = currentPos;

                dragStartRect = selectionRect;

                activeHandle = HandleType::BottomRight;
                isDrawing = true;

                update();
            }
        }
        return;
    }

    tempState = layers[currentLayerIndex];
    startPoint = QPoint(x, y);
    isDrawing = true;

    if (currentTool == DrawTool::Fill) {
        QColor targetColor = layers[currentLayerIndex].pixelColor(x, y);
        QColor replacementColor = (event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent;
        if (targetColor != replacementColor) {
            floodFill(x, y, targetColor, replacementColor);
            saveToHistory();
        }
    } else if (currentTool == DrawTool::Text) {
        bool ok;
        QString text = QInputDialog::getText(this, "Ввід тексту", "Введіть текст:", QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            QPainter p(&layers[currentLayerIndex]);
            p.setPen((event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent);
            QFont font("Arial", 6);
            p.setFont(font);
            p.drawText(x, y + 6, text);
            update();
            emit imageChanged(getFlattenedImage());
            saveToHistory();
        }
    } else if (currentTool == DrawTool::BrokenLine) {
        if (lastPoint.x() != -1) {
            QPainter p(&layers[currentLayerIndex]);
            p.setPen((event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent);
            p.drawLine(lastPoint, startPoint);
        }
        lastPoint = startPoint;
        update();
        emit imageChanged(getFlattenedImage());
    } else {
        mouseMoveEvent(event);
    }
}

void OledCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if (!isDrawing) return;

    if (currentTool == DrawTool::Pan) {
        setCursor(Qt::OpenHandCursor);
        isDrawing = false;
        return;
    }

    if (currentTool == DrawTool::Select) {
        activeHandle = HandleType::None;
        if (selectionRect.width() == 0 || selectionRect.height() == 0) {
            hasSelection = false;
        }
    } else {
        if (currentTool != DrawTool::Fill && currentTool != DrawTool::Text && currentTool != DrawTool::BrokenLine) {
            saveToHistory();
        }
    }

    isDrawing = false;
    update();
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

    if (hasSelection && selectionRect.width() > 0 && selectionRect.height() > 0) {

        painter.setClipRect(0, 0, 128, 64);

        if (isFloating && !floatingImage.isNull()) {
            painter.setOpacity(1.0);
            painter.drawImage(selectionRect.topLeft(), floatingImage);
        }

        QPen selPen(Qt::white, 1, Qt::DashLine);
        selPen.setCosmetic(true);
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(selPen);
        painter.drawRect(selectionRect);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        int hs = 1;
        QSize handleSize(hs * 2 + 1, hs * 2 + 1);
        QColor borderColor(139, 0, 0);
        QColor centerColor = Qt::black;

        QPoint corners[4] = {
            selectionRect.topLeft(),
            selectionRect.topRight(),
            selectionRect.bottomLeft(),
            selectionRect.bottomRight()
        };

        for (int i = 0; i < 4; ++i) {
            painter.fillRect(QRect(corners[i] - QPoint(hs, hs), handleSize), borderColor);
            painter.fillRect(QRect(corners[i], QSize(1, 1)), centerColor);
        }

        painter.setClipping(false);
    }

    painter.setOpacity(1.0);

    QPen gridPen(QColor(50, 50, 50));
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);

    for (int i = 0; i <= 128; ++i) painter.drawLine(i, 0, i, 64);
    for (int i = 0; i <= 64; ++i) painter.drawLine(0, i, 128, i);
}

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
        } else if (event->angleDelta().y() < 0) {
            scaleFactor /= zoomStep;
        }

        scaleFactor = qBound(1.0, scaleFactor, 40.0);
        if (qFuzzyCompare(scaleFactor, oldScale)) return;

        QPointF mousePos = event->position();
        QPointF worldPos = (mousePos - offset) / oldScale;
        offset = mousePos - worldPos * scaleFactor;

        clampOffset();
        update();
    }
    else {
        QPoint numPixels = event->pixelDelta();
        QPoint numDegrees = event->angleDelta() / 8;

        if (!numPixels.isNull()) {
            offset += numPixels;
        } else if (!numDegrees.isNull()) {
            offset += QPoint(numDegrees.x(), numDegrees.y());
        }

        clampOffset();
        update();
    }

    event->accept();
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

void OledCanvas::setTool(DrawTool tool) {
    if (currentTool == DrawTool::Select && tool != DrawTool::Select) {
        if (isFloating) {
            commitFloatingImage();
        }
        hasSelection = false;
    }

    currentTool = tool;

    if (tool != DrawTool::BrokenLine) {
        lastPoint = QPoint(-1, -1);
    }

    if (tool != DrawTool::Select) {
        selectionRect = QRect();
        hasSelection = false;
    }

    update();
}

void OledCanvas::floodFill(int x, int y, QColor targetColor, QColor replacementColor) {
    if (targetColor == replacementColor) return;

    QQueue<QPoint> queue;
    queue.enqueue(QPoint(x, y));

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();

        if (p.x() < 0 || p.x() >= 128 || p.y() < 0 || p.y() >= 64) continue;
        if (layers[currentLayerIndex].pixelColor(p) != targetColor) continue;

        layers[currentLayerIndex].setPixelColor(p, replacementColor);

        queue.enqueue(QPoint(p.x() + 1, p.y()));
        queue.enqueue(QPoint(p.x() - 1, p.y()));
        queue.enqueue(QPoint(p.x(), p.y() + 1));
        queue.enqueue(QPoint(p.x(), p.y() - 1));
    }
    update();
    emit imageChanged(getFlattenedImage());
}

void OledCanvas::commitFloatingImage() {
    if (!isFloating) return;

    QPainter p(&layers[currentLayerIndex]);
    p.drawImage(selectionRect.topLeft(), floatingImage);

    isFloating = false;
    hasSelection = false;
    saveToHistory();
    update();
    emit imageChanged(getFlattenedImage());
}

void OledCanvas::copyLayer() {
    if (hasSelection && selectionRect.width() > 0 && selectionRect.height() > 0) {
        internalClipboard = layers[currentLayerIndex].copy(selectionRect);

        for (int y = 0; y < internalClipboard.height(); ++y) {
            for (int x = 0; x < internalClipboard.width(); ++x) {
                if (internalClipboard.pixelColor(x, y) == Qt::black) {
                    internalClipboard.setPixelColor(x, y, Qt::transparent);
                }
            }
        }
    }

    setTool(DrawTool::Pen);
}

void OledCanvas::cutLayer() {
    QRect savedRect = selectionRect;

    copyLayer();

    if (savedRect.isEmpty()) return;

    tempState = layers[currentLayerIndex];
    QPainter p(&layers[currentLayerIndex]);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    QColor clearColor = (currentLayerIndex == 0) ? Qt::black : Qt::transparent;

    p.fillRect(savedRect, clearColor);

    saveToHistory();
    update();
    emit imageChanged(getFlattenedImage());
}

void OledCanvas::pasteToLayer() {
    if (internalClipboard.isNull()) return;

    if (isFloating) commitFloatingImage();

    floatingImage = internalClipboard;
    originalFloatingImage = internalClipboard;
    isFloating = true;
    hasSelection = true;

    selectionRect = QRect(0, 0, floatingImage.width(), floatingImage.height());

    setTool(DrawTool::Select);
}

void OledCanvas::rotateFloatingImage() {
    if (!isFloating) return;

    QTransform transform;
    transform.rotate(90);
    floatingImage = floatingImage.transformed(transform);
    originalFloatingImage = floatingImage;

    QPoint center = selectionRect.center();
    selectionRect.setSize(QSize(selectionRect.height(), selectionRect.width()));
    selectionRect.moveCenter(center);

    update();
}

HandleType OledCanvas::getHandleAt(const QPoint& pos) {
    if (!hasSelection) return HandleType::None;

    int tolerance = 3;

    QRect r = selectionRect;
    if (QLineF(pos, r.topLeft()).length() <= tolerance) return HandleType::TopLeft;
    if (QLineF(pos, r.topRight()).length() <= tolerance) return HandleType::TopRight;
    if (QLineF(pos, r.bottomLeft()).length() <= tolerance) return HandleType::BottomLeft;
    if (QLineF(pos, r.bottomRight()).length() <= tolerance) return HandleType::BottomRight;

    if (r.contains(pos)) return HandleType::Move;

    return HandleType::None;
}
