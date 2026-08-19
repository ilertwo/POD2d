#include "pixelcanvas.h"
#include "painttools.h"
#include <QMouseEvent>
#include <QInputDialog>
#include <QLineEdit>

constexpr int OVERSHOOT_MARGIN = 10;

PixelCanvas::PixelCanvas(QWidget *parent) : QWidget(parent) {
    scaleFactor = 5.0;
    setMouseTracking(true);
}

void PixelCanvas::setModel(ProjectModel *model) {
    m_model = model;
}

void PixelCanvas::mouseMoveEvent(QMouseEvent *event) {
    currentMousePos = event->pos();
    isMouseOnCanvas = true;

    if (!isDrawing) {
        if (currentTool == DrawTool::Brush) update();
        return;
    }

    if (currentTool == DrawTool::Pan) {
        offset += (event->pos() - lastPanPoint);
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
            selectionRect = dragStartRect.translated(currentPos - dragStartMousePos);
        } else {
            QRect newRect = dragStartRect;
            int dx = currentPos.x() - dragStartMousePos.x();
            int dy = currentPos.y() - dragStartMousePos.y();

            switch (activeHandle) {
            case HandleType::TopLeft:     newRect.setTopLeft(newRect.topLeft() + QPoint(dx, dy)); break;
            case HandleType::TopRight:    newRect.setTopRight(newRect.topRight() + QPoint(dx, dy)); break;
            case HandleType::BottomLeft:  newRect.setBottomLeft(newRect.bottomLeft() + QPoint(dx, dy)); break;
            case HandleType::BottomRight: newRect.setBottomRight(newRect.bottomRight() + QPoint(dx, dy)); break;
            default: break;
            }

            bool flipH = newRect.width() < 0;
            bool flipV = newRect.height() < 0;
            selectionRect = newRect.normalized();

            if (isFloating) {
                int newW = qMax(1, selectionRect.width());
                int newH = qMax(1, selectionRect.height());

                QImage scaled = originalFloatingImage.scaled(newW, newH, Qt::IgnoreAspectRatio, Qt::FastTransformation);
                floatingImage = (flipH || flipV) ? scaled.mirrored(flipH, flipV) : scaled;
            }
        }
        update();
        return;
    }

    if (!m_model) return;

    x = qBound(-OVERSHOOT_MARGIN, x, canvasWidth + OVERSHOOT_MARGIN);
    y = qBound(-OVERSHOOT_MARGIN, y, canvasHeight + OVERSHOOT_MARGIN);
    QPoint currentPoint(x, y);

    QColor drawColor = (event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent;

    if (m_model->getCurrentLayerIndex() == 0 && drawColor == Qt::transparent) {
        drawColor = Qt::black;
    }

    QImage &layerImg = m_model->getActiveLayerImage();


    switch (currentTool) {
    case DrawTool::Pen:
        break;
    case DrawTool::Brush:
        PaintTools::drawBrush(layerImg, x, y, brushSize, drawColor);
        break;

    case DrawTool::Line:
        layerImg = tempState;
        PaintTools::drawLine(layerImg, startPoint, currentPoint, drawColor);
        break;

    case DrawTool::Rectangle:
        layerImg = tempState;
        PaintTools::drawRect(layerImg, startPoint, currentPoint, drawColor);
        break;

    case DrawTool::Circle:
        layerImg = tempState;
        PaintTools::drawCircle(layerImg, startPoint, currentPoint, drawColor);
        break;

    default:
        break;
    }

    update();
    m_model->notifyImageChanged();
}

void PixelCanvas::mousePressEvent(QMouseEvent *event) {
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
    startPoint = currentPos;
    isDrawing = true;

    QImage &layerImg = m_model->getActiveLayerImage();
    const QColor replacementColor = (event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent;

    if (currentTool == DrawTool::Fill || currentTool == DrawTool::Dithering) {
        if (x >= 0 && x < layerImg.width() && y >= 0 && y < layerImg.height()) {
            QColor targetColor = layerImg.pixelColor(x, y);

            if (targetColor != replacementColor) {

                if (currentTool == DrawTool::Fill) {
                    PaintTools::floodFill(layerImg, x, y, targetColor, replacementColor);
                } else {
                    PaintTools::floodFillDithering(layerImg, x, y, targetColor, replacementColor);
                }

                m_model->saveHistoryStep(tempState);
                m_model->notifyImageChanged();
                update();
            }
        }
        return;
    }

    if (currentTool == DrawTool::BrokenLine) {
        if (lastPoint.x() != -1) {
            PaintTools::drawLine(layerImg, lastPoint, startPoint, replacementColor);
            m_model->saveHistoryStep(tempState);
        }
        lastPoint = startPoint;
        m_model->notifyImageChanged();
        update();
        return;
    }

    if (currentTool == DrawTool::Text) {
        bool ok;
        QString text = QInputDialog::getText(this, "Text input", "Enter text:", QLineEdit::Normal, "", &ok);

        if (ok && !text.isEmpty()) {
            QPainter p(&layerImg);

            p.setRenderHint(QPainter::TextAntialiasing, false);
            p.setRenderHint(QPainter::Antialiasing, false);

            QFont pixelFont("Arial", 6);
            pixelFont.setStyleStrategy(QFont::NoAntialias);
            pixelFont.setWeight(QFont::Light);

            p.setPen(replacementColor);
            p.setFont(pixelFont);

            p.drawText(x, y + 6, text);

            m_model->saveHistoryStep(tempState);
            m_model->notifyImageChanged();
            update();
        }

        isDrawing = false;
        return;
    }

    mouseMoveEvent(event);
}

void PixelCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if (!isDrawing) return;

    if (currentTool == DrawTool::Pan) {
        setCursor(Qt::OpenHandCursor);
        isDrawing = false;
        return;
    }

    if (currentTool == DrawTool::Select) {
        activeHandle = HandleType::None;

        if (selectionRect.isEmpty()) {
            hasSelection = false;
        }
    }
    else if (m_model) {
        switch (currentTool) {
        case DrawTool::Pen:
        case DrawTool::Brush:
        case DrawTool::Line:
        case DrawTool::Rectangle:
            m_model->saveHistoryStep(tempState);
            break;
        case DrawTool::Circle:
            m_model->saveHistoryStep(tempState);
            break;

        default:
            break;
        }
    }

    isDrawing = false;
    update();
}

void PixelCanvas::paintEvent(QPaintEvent *event) {

    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.save();

    painter.translate(offset);
    painter.scale(scaleFactor, scaleFactor);

    painter.fillRect(0, 0, canvasWidth, canvasHeight, Qt::black);

    if (m_model) {
        const QList<QImage> &currentLayers = m_model->getCurrentLayers();
        const int activeIdx = m_model->getCurrentLayerIndex();

        for (int i = 0; i < currentLayers.size(); ++i) {
            if (i < activeIdx) {
                const int distance = activeIdx - i;
                const double opacity = qMax(0.1, 1.0 - (distance * 0.3));
                painter.setOpacity(opacity);
            } else if (i == activeIdx) {
                painter.setOpacity(1.0);
            } else {
                continue;
            }
            painter.drawImage(0, 0, currentLayers[i]);
        }
    }

    painter.setOpacity(1.0);

    if (hasSelection && !selectionRect.isEmpty()) {
        painter.setClipRect(0, 0, canvasWidth, canvasHeight);

        if (isFloating && !floatingImage.isNull()) {
            painter.drawImage(selectionRect.topLeft(), floatingImage);
        }

        QPen selPen(Qt::white, 1, Qt::DashLine);
        selPen.setCosmetic(true);
        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(selPen);
        painter.drawRect(selectionRect);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        const int hs = 1;
        const QSize handleSize(hs * 2 + 1, hs * 2 + 1);

        const QPoint corners[4] = {
            selectionRect.topLeft(), selectionRect.topRight(),
            selectionRect.bottomLeft(), selectionRect.bottomRight()
        };

        for (int i = 0; i < 4; ++i) {
            painter.fillRect(QRect(corners[i] - QPoint(hs, hs), handleSize), QColor(139, 0, 0));
            painter.fillRect(QRect(corners[i], QSize(1, 1)), Qt::black);
        }
        painter.setClipping(false);
    }

    if (scaleFactor >= 4.0) {
        QPen gridPen(QColor(50, 50, 50));
        gridPen.setCosmetic(true);
        painter.setPen(gridPen);

        QVector<QLine> gridLines;
        gridLines.reserve(canvasWidth + canvasHeight + 2);
        for (int i = 0; i <= canvasWidth; ++i) gridLines.append(QLine(i, 0, i, canvasHeight));
        for (int i = 0; i <= canvasHeight; ++i) gridLines.append(QLine(0, i, canvasWidth, i));

        painter.drawLines(gridLines);
    }

    painter.restore();

    if (isMouseOnCanvas && currentTool == DrawTool::Brush && brushSize > 0) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(180, 180, 180), 1));
        painter.setBrush(Qt::NoBrush);

        const float radius = (brushSize * scaleFactor) / 2.0f;
        painter.drawEllipse(currentMousePos, qRound(radius), qRound(radius));
    }
}

void PixelCanvas::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        constexpr double zoomStep = 1.15;
        const double oldScale = scaleFactor;

        if (event->angleDelta().y() > 0) {
            scaleFactor *= zoomStep;
        } else if (event->angleDelta().y() < 0) {
            scaleFactor /= zoomStep;
        }

        scaleFactor = qBound(1.0, scaleFactor, 40.0);

        if (qFuzzyCompare(scaleFactor, oldScale)) return;

        const QPointF mousePos = event->position();
        const QPointF worldPos = (mousePos - offset) / oldScale;
        offset = mousePos - worldPos * scaleFactor;

        clampOffset();
        update();
    }
    else {
        const QPoint numPixels = event->pixelDelta();
        const QPoint numDegrees = event->angleDelta() / 8;

        if (!numPixels.isNull()) {
            offset += numPixels;
        } else if (!numDegrees.isNull()) {
            offset += numDegrees * 3;
        }

        clampOffset();
        update();
    }

    event->accept();
}

void PixelCanvas::setZoom(double newScale) {
    double clampedScale = qBound(1.0, newScale, 40.0);

    if (qAbs(clampedScale - scaleFactor) < 0.001) return;

    QPointF screenCenter = rect().center();
    QPointF canvasCenter = (screenCenter - offset) / scaleFactor;

    scaleFactor = clampedScale;
    offset = (screenCenter - (canvasCenter * scaleFactor)).toPoint();

    clampOffset();
    update();
}

void PixelCanvas::clampOffset() {
    const double currentWidth = canvasWidth * scaleFactor;
    const double currentHeight = canvasHeight * scaleFactor;

    const double marginX = currentWidth * 0.2;
    const double marginY = currentHeight * 0.2;

    const double minX = -currentWidth + marginX;
    const double minY = -currentHeight + marginY;

    const double maxX = width() - marginX;
    const double maxY = height() - marginY;

    offset.setX(qBound(minX, offset.x(), maxX));
    offset.setY(qBound(minY, offset.y(), maxY));
}

void PixelCanvas::setTool(DrawTool tool) {
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

void PixelCanvas::commitFloatingImage() {
    if (!isFloating || !m_model) return;

    m_model->commitImageToCurrentLayer(selectionRect.topLeft(), floatingImage);

    isFloating = false;
    hasSelection = false;
    update();
}

void PixelCanvas::rotateFloatingImage() {
    if (!isFloating) return;

    QTransform transform;
    transform.rotate(90);
    floatingImage = floatingImage.transformed(transform);
    originalFloatingImage = floatingImage;

    const QPoint center = selectionRect.center();
    selectionRect.setSize(QSize(selectionRect.height(), selectionRect.width()));
    selectionRect.moveCenter(center);

    update();
}

HandleType PixelCanvas::getHandleAt(const QPoint& pos) {
    if (!hasSelection) return HandleType::None;

    const int tol = 3;
    const QRect r = selectionRect;

    if ((pos - r.topLeft()).manhattanLength() <= tol) return HandleType::TopLeft;
    if ((pos - r.topRight()).manhattanLength() <= tol) return HandleType::TopRight;
    if ((pos - r.bottomLeft()).manhattanLength() <= tol) return HandleType::BottomLeft;
    if ((pos - r.bottomRight()).manhattanLength() <= tol) return HandleType::BottomRight;

    if (r.contains(pos)) return HandleType::Move;

    return HandleType::None;
}

void PixelCanvas::leaveEvent(QEvent *event) {
    isMouseOnCanvas = false;
    update();
    QWidget::leaveEvent(event);
}

void PixelCanvas::pasteToLayer() {
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

void PixelCanvas::copyLayer() {
    if (!m_model || !hasSelection || selectionRect.isEmpty()) {
        return;
    }

    const QImage &layer = m_model->getActiveLayerImage();
    QImage clip = layer.copy(selectionRect).convertToFormat(QImage::Format_ARGB32);

    const QRgb transparentRgb = qRgba(0, 0, 0, 0);

    for (int y = 0; y < clip.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(clip.scanLine(y));

        for (int x = 0; x < clip.width(); ++x) {
            if (qRed(line[x]) == 0 && qGreen(line[x]) == 0 && qBlue(line[x]) == 0) {
                line[x] = transparentRgb;
            }
        }
    }

    m_model->setClipboardImage(clip);
}

void PixelCanvas::cutLayer() {
    if (!m_model || !hasSelection || selectionRect.isEmpty()) return;

    const QRect savedRect = selectionRect;

    copyLayer();

    m_model->saveHistoryStep(tempState);

    m_model->clearRectOnCurrentLayer(savedRect);

    hasSelection = false;
    selectionRect = QRect();

    m_model->notifyImageChanged();
    update();
}

void PixelCanvas::setBrushSize(int size) {
    brushSize = qMax(1, size);
    update();
}

int PixelCanvas::getBrushSize() const {
    return brushSize;
}

void PixelCanvas::fitToScreen() {
    double canvasWidthDouble = canvasWidth;
    double canvasHeightDouble = canvasHeight;
    double scaleX = (this->width() * 0.85) / canvasWidthDouble;
    double scaleY = (this->height() * 0.85) / canvasHeightDouble;

    scaleFactor = qBound(1.0, qMin(scaleX, scaleY), 40.0);

    const double canvasPhysicalWidth = canvasWidthDouble * scaleFactor;
    const double canvasPhysicalHeight = canvasHeightDouble * scaleFactor;

    double offsetX = (this->width() - canvasPhysicalWidth) / 2.0;
    double offsetY = (this->height() - canvasPhysicalHeight) / 2.0;

    offset = QPointF(offsetX, offsetY);

    update();
}

void PixelCanvas::resetToolState() {
    lastPoint = QPoint(-1, -1);
}

void PixelCanvas::setCanvasSize(int width, int height) {
    canvasWidth = width;
    canvasHeight = height;

    update();
}
