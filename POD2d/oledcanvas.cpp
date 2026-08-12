#include "oledcanvas.h"
#include "painttools.h"

OledCanvas::OledCanvas(QWidget *parent) : QWidget(parent) {
    scaleFactor = 5.0;

    Frame firstFrame;
    QImage backgroundLayer(128, 64, QImage::Format_ARGB32);
    backgroundLayer.fill(Qt::black);
    firstFrame.layers.append(backgroundLayer);

    setMouseTracking(true);
}

void OledCanvas::setModel(ProjectModel *model) { m_model = model; }

void OledCanvas::mouseMoveEvent(QMouseEvent *event) {
    currentMousePos = event->pos();
    isMouseOnCanvas = true;

    if (!isDrawing) {
        if (currentTool == DrawTool::Brush) update();
        return;
    }

    if (currentTool == DrawTool::Pan) {
        QPoint delta = event->pos() - lastPanPoint;
        offset += delta;
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
                if (flipH || flipV) scaled = scaled.mirrored(flipH, flipV);
                floatingImage = scaled;
            }
        }
        update();
        return;
    }

    if (!m_model) return;

    x = qBound(-10, x, 138);
    y = qBound(-10, y, 74);
    QPoint currentPoint(x, y);

    QColor drawColor = (event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent;
    if (m_model->getCurrentLayerIndex() == 0 && drawColor == Qt::transparent) {
        drawColor = Qt::black;
    }

    QImage &layerImg = m_model->getActiveLayerImage();

    if (currentTool == DrawTool::Pen || currentTool == DrawTool::Brush) {
        PaintTools::drawBrush(layerImg, x, y, brushSize, drawColor);
    }
    else if (currentTool == DrawTool::Line || currentTool == DrawTool::Rectangle || currentTool == DrawTool::Circle) {
        layerImg = tempState;

        if (currentTool == DrawTool::Line) {
            PaintTools::drawLine(layerImg, startPoint, currentPoint, drawColor);
        } else if (currentTool == DrawTool::Rectangle) {
            PaintTools::drawRect(layerImg, startPoint, currentPoint, drawColor);
        } else if (currentTool == DrawTool::Circle) {
            PaintTools::drawCircle(layerImg, startPoint, currentPoint, drawColor);
        }
    }

    update();
    m_model->notifyImageChanged();
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
                selectionRect = QRect(currentPos, QSize(0, 0));
                dragStartMousePos = currentPos;
                dragStartRect = selectionRect;
                activeHandle = HandleType::BottomRight;
                isDrawing = true;
                update();
            }
        }
        return;
    }

    if (!m_model) return;

    tempState = m_model->getActiveLayerImage();
    startPoint = QPoint(x, y);
    isDrawing = true;

    QImage &layerImg = m_model->getActiveLayerImage();
    QColor replacementColor = (event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent;

    if (currentTool == DrawTool::Fill) {
        if (x >= 0 && x < layerImg.width() && y >= 0 && y < layerImg.height()) {
            QColor targetColor = layerImg.pixelColor(x, y);
            if (targetColor != replacementColor) {
                PaintTools::floodFill(layerImg, x, y, targetColor, replacementColor);
                m_model->saveHistoryStep();
                m_model->notifyImageChanged();
                update();
            }
        }
    }
    else if (currentTool == DrawTool::Dithering) {
        if (x >= 0 && x < layerImg.width() && y >= 0 && y < layerImg.height()) {
            QColor targetColor = layerImg.pixelColor(x, y);
            if (targetColor != replacementColor) {
                PaintTools::floodFillDithering(layerImg, x, y, targetColor, replacementColor);
                m_model->saveHistoryStep();
                m_model->notifyImageChanged();
                update();
            }
        }
    }
    else if (currentTool == DrawTool::Text) {
        bool ok;
        QString text = QInputDialog::getText(this, "Ввід тексту", "Введіть текст:", QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            QPainter p(&layerImg);
            p.setPen(replacementColor);
            p.setFont(QFont("Arial", 6));
            p.drawText(x, y + 6, text);

            m_model->saveHistoryStep();
            m_model->notifyImageChanged();
            update();
        }
    }
    else if (currentTool == DrawTool::BrokenLine) {
        if (lastPoint.x() != -1) {
            PaintTools::drawLine(layerImg, lastPoint, startPoint, replacementColor);
        }
        lastPoint = startPoint;
        m_model->notifyImageChanged();
        update();
    }
    else {
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
    } else if (m_model) {
        if (currentTool != DrawTool::Fill && currentTool != DrawTool::Text &&
            currentTool != DrawTool::BrokenLine && currentTool != DrawTool::Dithering) {
            m_model->saveHistoryStep();
            m_model->notifyImageChanged();
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

    if (m_model) {
        const QList<QImage> &currentLayers = m_model->getCurrentLayers();
        int activeIdx = m_model->getCurrentLayerIndex();

        for (int i = 0; i < currentLayers.size(); ++i) {
            if (i < activeIdx) {
                int distance = activeIdx - i;
                double opacity = qMax(0.1, 1.0 - (distance * 0.3));
                painter.setOpacity(opacity);
            } else if (i == activeIdx) {
                painter.setOpacity(1.0);
            } else {
                painter.setOpacity(0.0);
            }
            painter.drawImage(0, 0, currentLayers[i]);
        }
    }

    painter.setOpacity(1.0);

    if (hasSelection && selectionRect.width() > 0 && selectionRect.height() > 0) {
        painter.setClipRect(0, 0, 128, 64);

        if (isFloating && !floatingImage.isNull()) {
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

        QPoint corners[4] = {
            selectionRect.topLeft(), selectionRect.topRight(),
            selectionRect.bottomLeft(), selectionRect.bottomRight()
        };

        for (int i = 0; i < 4; ++i) {
            painter.fillRect(QRect(corners[i] - QPoint(hs, hs), handleSize), QColor(139, 0, 0));
            painter.fillRect(QRect(corners[i], QSize(1, 1)), Qt::black);
        }
        painter.setClipping(false);
    }

    // Сітка
    QPen gridPen(QColor(50, 50, 50));
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);
    for (int i = 0; i <= 128; ++i) painter.drawLine(i, 0, i, 64);
    for (int i = 0; i <= 64; ++i) painter.drawLine(0, i, 128, i);

    if (isMouseOnCanvas && currentTool == DrawTool::Brush && brushSize > 0) {
        QPainter screenPainter(this);
        screenPainter.setRenderHint(QPainter::Antialiasing, true);
        screenPainter.setPen(QPen(QColor(180, 180, 180), 1));
        screenPainter.setBrush(Qt::NoBrush);

        float radius = (brushSize * scaleFactor) / 2.0f;
        screenPainter.drawEllipse(currentMousePos, qRound(radius), qRound(radius));
    }
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

void OledCanvas::commitFloatingImage() {
    if (!isFloating || !m_model) return;

    m_model->commitImageToCurrentLayer(selectionRect.topLeft(), floatingImage);

    isFloating = false;
    hasSelection = false;
    update();
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

void OledCanvas::leaveEvent(QEvent *event) {
    isMouseOnCanvas = false;
    update();
    QWidget::leaveEvent(event);
}

void OledCanvas::pasteToLayer() {
    if (!m_model) return;

    QImage clip = m_model->getClipboardImage();
    if (clip.isNull()) return;

    if (isFloating) {
        commitFloatingImage();
    }

    floatingImage = clip;
    originalFloatingImage = clip;
    isFloating = true;
    hasSelection = true;

    selectionRect = QRect(0, 0, floatingImage.width(), floatingImage.height());

    setTool(DrawTool::Select);
    update();
}

void OledCanvas::copyLayer() {
    if (!m_model || !hasSelection || selectionRect.width() <= 0 || selectionRect.height() <= 0) {
        return;
    }

    QImage &layer = m_model->getActiveLayerImage();
    QImage clip = layer.copy(selectionRect);

    for (int y = 0; y < clip.height(); ++y) {
        for (int x = 0; x < clip.width(); ++x) {
            if (clip.pixelColor(x, y) == Qt::black) {
                clip.setPixelColor(x, y, Qt::transparent);
            }
        }
    }

    m_model->setClipboardImage(clip);
    setTool(DrawTool::Pen);
}

void OledCanvas::cutLayer() {
    if (!hasSelection || selectionRect.isEmpty() || !m_model) return;

    QRect savedRect = selectionRect;

    copyLayer();

    m_model->clearRectOnCurrentLayer(savedRect);

    hasSelection = false;
    selectionRect = QRect();
    update();
}

void OledCanvas::setBrushSize(int size) {
    brushSize = qMax(1, size);
    update();
}
int OledCanvas::getBrushSize() const { return brushSize; }

