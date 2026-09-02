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
        if (currentTool == DrawTool::Brush ||
            currentTool == DrawTool::Eraser ||
            currentTool == DrawTool::Lighten) {
            update();
        }
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
    QPoint currentPoint(x, y);
    emit cursorPositionChanged(x, y);

    if (currentTool == DrawTool::Select && activeHandle != HandleType::None) {
        QPoint currentPos(x, y);

        if (activeHandle == HandleType::Move) {
            QPoint offsetMove = currentPos - dragStartMousePos;
            selectionRect = dragStartRect.translated(offsetMove);

            if (!dragStartPath.isEmpty()) {
                QTransform t;
                t.translate(offsetMove.x(), offsetMove.y());
                selectionPath = t.map(dragStartPath);
            }
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

            if (!dragStartPath.isEmpty() && dragStartRect.width() != 0 && dragStartRect.height() != 0) {
                QTransform t;
                t.translate(selectionRect.x(), selectionRect.y());
                t.scale(selectionRect.width() / (double)dragStartRect.width(),
                        selectionRect.height() / (double)dragStartRect.height());
                t.translate(-dragStartRect.x(), -dragStartRect.y());
                selectionPath = t.map(dragStartPath);
            }

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

    if (currentTool == DrawTool::Pipette && (event->buttons() & (Qt::LeftButton | Qt::RightButton))) {
        if (m_model && x >= 0 && x < canvasWidth && y >= 0 && y < canvasHeight) {
            QColor picked = m_model->getActiveLayerImage().pixelColor(x, y);

            if (!m_model->getIsRGB() && picked == Qt::white) {
                picked = m_monoColor;
            }
            if (event->buttons() & Qt::LeftButton) {
                primaryColor = picked;
                emit colorPicked(picked, true);
            } else {
                secondaryColor = picked;
                emit colorPicked(picked, false);
            }
        }
        return;
    }

    if (isDrawing && currentTool == DrawTool::LassoSelect) {
        selectionPath.lineTo(currentPoint);
        update();
        return;
    }

    if (isDrawing && currentTool == DrawTool::ShapeSelect) {
        selectionPath = QPainterPath();

        int left = qMin(startPoint.x(), currentPoint.x());
        int right = qMax(startPoint.x(), currentPoint.x());
        int top = qMin(startPoint.y(), currentPoint.y());
        int bottom = qMax(startPoint.y(), currentPoint.y());

        QRect rect(left, top, right - left + 1, bottom - top + 1);
        selectionPath.addEllipse(rect);

        update();
        return;
    }

    if (!m_model) return;

    x = qBound(-OVERSHOOT_MARGIN, x, canvasWidth + OVERSHOOT_MARGIN);
    y = qBound(-OVERSHOOT_MARGIN, y, canvasHeight + OVERSHOOT_MARGIN);

    QColor drawColor;

    if (currentTool == DrawTool::Eraser) {
        if (!m_model->getIsRGB() && m_model->getCurrentLayerIndex() == 0) {
            drawColor = Qt::black;
        } else {
            drawColor = Qt::transparent;
        }
    } else {
        if (event->buttons() & Qt::LeftButton) {
            drawColor = primaryColor;
        } else if (event->buttons() & Qt::RightButton) {
            drawColor = secondaryColor;
        } else {
            return;
        }
    }

    QImage &layerImg = m_model->getActiveLayerImage();

    if (!m_model->getIsRGB() && drawColor == m_monoColor) {
        drawColor = Qt::white;
    }

    switch (currentTool) {
    case DrawTool::Pen:
        break;
    case DrawTool::Eraser:
    case DrawTool::Brush:
        PaintTools::drawBrush(layerImg, x, y, brushSize, drawColor);
        if (m_verticalMirror) {
            PaintTools::drawBrush(layerImg, canvasWidth - 1 - x, y, brushSize, drawColor);
        }
        break;

    case DrawTool::Lighten: {
        bool isRightClick = (event->buttons() & Qt::RightButton);
        PaintTools::lightenBrush(layerImg, x, y, brushSize, isRightClick);
        if (m_verticalMirror) {
            PaintTools::lightenBrush(layerImg, canvasWidth - 1 - x, y, brushSize, isRightClick);
        }
        break;
    }
    case DrawTool::Line:
        layerImg = tempState;
        PaintTools::drawLine(layerImg, startPoint, currentPoint, drawColor);
        if (m_verticalMirror) {
            PaintTools::drawLine(layerImg, getMirroredPoint(startPoint), getMirroredPoint(currentPoint), drawColor);
        }
        break;

    case DrawTool::Rectangle:
        layerImg = tempState;
        PaintTools::drawRect(layerImg, startPoint, currentPoint, drawColor);
        if (m_verticalMirror) {
            PaintTools::drawRect(layerImg, getMirroredPoint(startPoint), getMirroredPoint(currentPoint), drawColor);
        }
        break;

    case DrawTool::Circle:
        layerImg = tempState;
        PaintTools::drawCircle(layerImg, startPoint, currentPoint, drawColor);
        if (m_verticalMirror) {
            PaintTools::drawCircle(layerImg, getMirroredPoint(startPoint), getMirroredPoint(currentPoint), drawColor);
        }
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

    if (currentTool == DrawTool::Pipette) {
        if (m_model && x >= 0 && x < canvasWidth && y >= 0 && y < canvasHeight) {
            QColor picked = m_model->getActiveLayerImage().pixelColor(x, y);
            if (!m_model->getIsRGB() && picked == Qt::white) {
                picked = m_monoColor;
            }
            if (event->buttons() & Qt::LeftButton) {
                primaryColor = picked;
                emit colorPicked(picked, true);
            } else if (event->buttons() & Qt::RightButton) {
                secondaryColor = picked;
                emit colorPicked(picked, false);
            }
        }
        return;
    }

    if (currentTool == DrawTool::Select) {
        activeHandle = getHandleAt(currentPos);
        if (activeHandle != HandleType::None) {
            dragStartMousePos = currentPos;
            dragStartRect = selectionRect;
            dragStartPath = selectionPath;
            isDrawing = true;
        } else {
            if (isFloating) {
                commitFloatingImage();
            } else {
                hasSelection = true;
                selectionRect = QRect(currentPos, QSize(1, 1));
                selectionPath = QPainterPath();
                dragStartMousePos = currentPos;
                dragStartRect = selectionRect;
                activeHandle = HandleType::BottomRight;
                isDrawing = true;
                update();
            }
        }
        return;
    }

    if (currentTool == DrawTool::LassoSelect || currentTool == DrawTool::ShapeSelect) {
        if (isFloating) commitFloatingImage();
        hasSelection = false;
        selectionPath = QPainterPath();

        if (currentTool == DrawTool::LassoSelect) {
            selectionPath.moveTo(currentPos);
        }
        startPoint = currentPos;
        isDrawing = true;
        return;
    }

    if (!m_model) return;

    tempState = m_model->getActiveLayerImage();
    startPoint = currentPos;
    isDrawing = true;

    QImage &layerImg = m_model->getActiveLayerImage();
    QColor replacementColor;

    if (currentTool == DrawTool::Eraser) {
        if (!m_model->getIsRGB() && m_model->getCurrentLayerIndex() == 0) {
            replacementColor = Qt::black;
        } else {
            replacementColor = Qt::transparent;
        }
    } else {
        if (event->buttons() & Qt::LeftButton) {
            replacementColor = primaryColor;
        } else if (event->buttons() & Qt::RightButton) {
            replacementColor = secondaryColor;
        } else {
            return;
        }
    }

    if (!m_model->getIsRGB() && replacementColor == m_monoColor) {
        replacementColor = Qt::white;
    }
    
    if (currentTool == DrawTool::Fill || currentTool == DrawTool::Dithering) {
        if (x >= 0 && x < layerImg.width() && y >= 0 && y < layerImg.height()) {
            QColor targetColor = layerImg.pixelColor(x, y);

            if (targetColor != replacementColor) {
                if (currentTool == DrawTool::Fill) {
                    PaintTools::floodFill(layerImg, x, y, targetColor, replacementColor);
                } else {
                    PaintTools::floodFillDithering(layerImg, x, y, targetColor, replacementColor);
                }

                if (m_verticalMirror) {
                    int mx = canvasWidth - 1 - x;
                    if (mx >= 0 && mx < layerImg.width() && y >= 0 && y < layerImg.height()) {
                        QColor mirrorTarget = layerImg.pixelColor(mx, y);
                        if (mirrorTarget != replacementColor) {
                            if (currentTool == DrawTool::Fill) PaintTools::floodFill(layerImg, mx, y, mirrorTarget, replacementColor);
                            else PaintTools::floodFillDithering(layerImg, mx, y, mirrorTarget, replacementColor);
                        }
                    }
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

    if (currentTool == DrawTool::LassoSelect || currentTool == DrawTool::ShapeSelect) {
        if (currentTool == DrawTool::LassoSelect) {
            selectionPath.closeSubpath();
        }

        selectionRect = selectionPath.boundingRect().toRect();

        if (selectionRect.width() == 0) selectionRect.setWidth(1);
        if (selectionRect.height() == 0) selectionRect.setHeight(1);

        hasSelection = !selectionRect.isEmpty();
        isDrawing = false;

        if (hasSelection) setTool(DrawTool::Select);
        update();
        return;
    }

    else if (m_model) {
        switch (currentTool) {
        case DrawTool::Pen:
        case DrawTool::Eraser:
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

    if (m_bgStyle == "Solid Black") {
        painter.fillRect(0, 0, canvasWidth, canvasHeight, Qt::black);
    }
    else if (m_bgStyle == "Solid White") {
        painter.fillRect(0, 0, canvasWidth, canvasHeight, Qt::white);
    }
    else {
        const int checkerSize = 4;
        for (int y = 0; y < canvasHeight; y += checkerSize) {
            for (int x = 0; x < canvasWidth; x += checkerSize) {
                QColor color = ((x / checkerSize + y / checkerSize) % 2 == 0)
                ? QColor(150, 150, 150)
                : QColor(100, 100, 100);
                int w = qMin(checkerSize, canvasWidth - x);
                int h = qMin(checkerSize, canvasHeight - y);
                painter.fillRect(x, y, w, h, color);
            }
        }
    }

    if (m_model) {
        const QList<QImage> &currentLayers = m_model->getCurrentLayers();
        const int activeIdx = m_model->getCurrentLayerIndex();

        for (int i = 0; i < currentLayers.size(); ++i) {
            QImage layerToDraw = currentLayers[i];

            if (!m_model->getIsRGB() && m_monoColor != Qt::white) {
                layerToDraw = layerToDraw.convertToFormat(QImage::Format_ARGB32);
                const QRgb whiteRgb = qRgba(255, 255, 255, 255);
                const QRgb targetRgb = m_monoColor.rgba();

                for (int y = 0; y < layerToDraw.height(); ++y) {
                    QRgb *line = reinterpret_cast<QRgb*>(layerToDraw.scanLine(y));
                    for (int x = 0; x < layerToDraw.width(); ++x) {
                        if (line[x] == whiteRgb) {
                            line[x] = targetRgb;
                        }
                    }
                }
            }

            if (i < activeIdx) {
                const int distance = activeIdx - i;
                const double opacity = qMax(0.1, 1.0 - (distance * 0.3));
                painter.setOpacity(opacity);
            } else if (i == activeIdx) {
                painter.setOpacity(1.0);
            } else {
                continue;
            }
            painter.drawImage(0, 0, layerToDraw);
        }
    }

    painter.setOpacity(1.0);

    if (hasSelection && (!selectionRect.isEmpty() || !selectionPath.isEmpty())) {
        painter.setClipRect(0, 0, canvasWidth, canvasHeight);

        if (isFloating && !floatingImage.isNull()) {
            painter.drawImage(selectionRect.topLeft(), floatingImage);
        }

        QVector<qreal> dashes;
        dashes << 3 << 3;

        QPen selPen(Qt::white, 2);
        selPen.setDashPattern(dashes);
        selPen.setCosmetic(true);

        painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
        painter.setPen(selPen);

        if (!selectionPath.isEmpty()) {
            painter.drawPath(selectionPath);
        } else {
            painter.drawRect(selectionRect);
        }

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        if (!selectionRect.isEmpty()) {
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
        }
        painter.setClipping(false);
    }

    if (m_showGrid && scaleFactor >= 4.0) {
        QPen gridPen(m_gridColor);
        gridPen.setCosmetic(true);
        painter.setPen(gridPen);

        QVector<QLine> gridLines;
        gridLines.reserve(canvasWidth + canvasHeight + 2);
        for (int i = 0; i <= canvasWidth; ++i) gridLines.append(QLine(i, 0, i, canvasHeight));
        for (int i = 0; i <= canvasHeight; ++i) gridLines.append(QLine(0, i, canvasWidth, i));

        painter.drawLines(gridLines);
    }

    painter.restore();

    if (isMouseOnCanvas && (currentTool == DrawTool::Brush || currentTool == DrawTool::Eraser || currentTool == DrawTool::Lighten) && brushSize > 0) {
        painter.setRenderHint(QPainter::Antialiasing, true);

        if (currentTool == DrawTool::Lighten) {
            painter.setPen(QPen(QColor(160, 140, 0, 180), 1, Qt::DashLine));
        } else if (currentTool == DrawTool::Eraser) {
            painter.setPen(QPen(QColor(160, 60, 60, 180), 1));
        } else {
            painter.setPen(QPen(QColor(180, 180, 180), 1));
        }

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
        selectionPath = QPainterPath();
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

    m_model->notifyImageChanged();
    update();
}

void PixelCanvas::setPrimaryColor(const QColor &color) {
    primaryColor = color;
}

void PixelCanvas::setSecondaryColor(const QColor &color) {
    secondaryColor = color;
}

QColor PixelCanvas::getPrimaryColor() const { return primaryColor; }

QColor PixelCanvas::getSecondaryColor() const { return secondaryColor; }

void PixelCanvas::setShowGrid(bool show) {
    m_showGrid = show;
    update();
}

void PixelCanvas::setGridColor(const QColor &color) {
    m_gridColor = color;
    update();
}

void PixelCanvas::setBackgroundStyle(const QString &style) {
    m_bgStyle = style;
    update();
}

void PixelCanvas::setMonoDisplayColor(const QColor &color) {
    m_monoColor = color;
    update();
}

QColor PixelCanvas::getMonoDisplayColor() const {
    return m_monoColor;
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

    emit cursorPositionChanged(-1, -1);
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

    selectionPath = QPainterPath();
    selectionRect = QRect(0, 0, floatingImage.width(), floatingImage.height());

    setTool(DrawTool::Select);
    update();
}

void PixelCanvas::copyLayer() {
    if (!m_model || !hasSelection || selectionRect.isEmpty()) {
        return;
    }

    const QImage &layer = m_model->getActiveLayerImage();

    QImage clip(selectionRect.size(), QImage::Format_ARGB32);
    clip.fill(Qt::transparent);

    QPainter painter(&clip);
    painter.setRenderHint(QPainter::Antialiasing, false);

    if (!selectionPath.isEmpty()) {
        QPainterPath shiftedPath = selectionPath;
        shiftedPath.translate(-selectionRect.topLeft());
        painter.setClipPath(shiftedPath);
    }

    painter.drawImage(0, 0, layer, selectionRect.x(), selectionRect.y(), selectionRect.width(), selectionRect.height());
    painter.end();

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

    copyLayer();
    m_model->saveHistoryStep(tempState);

    QImage &layer = m_model->getActiveLayerImage();
    QPainter p(&layer);
    p.setCompositionMode(QPainter::CompositionMode_Clear);

    if (!selectionPath.isEmpty()) {
        p.fillPath(selectionPath, Qt::transparent);
    } else {
        p.fillRect(selectionRect, Qt::transparent);
    }
    p.end();

    hasSelection = false;
    selectionRect = QRect();
    selectionPath = QPainterPath();

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

    hasSelection = false;
    isFloating = false;
    selectionRect = QRect();
    selectionPath = QPainterPath();
    floatingImage = QImage();
}

void PixelCanvas::setCanvasSize(int width, int height) {
    canvasWidth = width;
    canvasHeight = height;

    update();
}

void PixelCanvas::setVerticalMirror(bool enabled) {
    m_verticalMirror = enabled;
    update();
}

QPoint PixelCanvas::getMirroredPoint(const QPoint &pt) const {
    return QPoint(canvasWidth - 1 - pt.x(), pt.y());
}
