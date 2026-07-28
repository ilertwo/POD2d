#include "oledcanvas.h"

OledCanvas::OledCanvas(QWidget *parent) : QWidget(parent) {
    scaleFactor = 5.0;

    Frame firstFrame;
    QImage backgroundLayer(128, 64, QImage::Format_ARGB32);
    backgroundLayer.fill(Qt::black);
    firstFrame.layers.append(backgroundLayer);

    frames.append(firstFrame);
    currentFrameIndex = 0;
    currentLayerIndex = 0;

    setMinimumSize(128, 64);
    saveToHistory();

    playTimer = new QTimer(this);
    connect(playTimer, &QTimer::timeout, this, &OledCanvas::onPlayTimerTick);
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
            frames[currentFrameIndex].layers[currentLayerIndex].setPixelColor(x, y, drawColor);
        }
    }
    else if (currentTool == DrawTool::Line || currentTool == DrawTool::Rectangle || currentTool == DrawTool::Circle) {
        frames[currentFrameIndex].layers[currentLayerIndex] = tempState;

        QPainter p(&frames[currentFrameIndex].layers[currentLayerIndex]);
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

    tempState = frames[currentFrameIndex].layers[currentLayerIndex];
    startPoint = QPoint(x, y);
    isDrawing = true;

    if (currentTool == DrawTool::Fill) {
        QColor targetColor = frames[currentFrameIndex].layers[currentLayerIndex].pixelColor(x, y);
        QColor replacementColor = (event->buttons() & Qt::LeftButton) ? Qt::white : Qt::transparent;
        if (targetColor != replacementColor) {
            floodFill(x, y, targetColor, replacementColor);
            saveToHistory();
        }
    } else if (currentTool == DrawTool::Text) {
        bool ok;
        QString text = QInputDialog::getText(this, "Ввід тексту", "Введіть текст:", QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            QPainter p(&frames[currentFrameIndex].layers[currentLayerIndex]);
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
            QPainter p(&frames[currentFrameIndex].layers[currentLayerIndex]);
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
            emit imageChanged(getFlattenedImage());
        }
    }

    isDrawing = false;
    update();

    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);
}

void OledCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.translate(offset);
    painter.scale(scaleFactor, scaleFactor);

    painter.fillRect(0, 0, 128, 64, Qt::black);

    QList<QImage>& currentLayers = frames[currentFrameIndex].layers;

    for (int i = 0; i < currentLayers.size(); ++i) {

        if (i < currentLayerIndex) {
            int distance = currentLayerIndex - i;
            double opacity = qMax(0.1, 1.0 - (distance * 0.3));
            painter.setOpacity(opacity);
        }
        else if (i == currentLayerIndex) {
            painter.setOpacity(1.0);
        }
        else {
            painter.setOpacity(0.0);
        }

        painter.drawImage(0, 0, currentLayers[i]);
    }

    painter.setOpacity(1.0);

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

    QPen gridPen(QColor(50, 50, 50));
    gridPen.setCosmetic(true);
    painter.setPen(gridPen);

    for (int i = 0; i <= 128; ++i) painter.drawLine(i, 0, i, 64);
    for (int i = 0; i <= 64; ++i) painter.drawLine(0, i, 128, i);
}

QVector<uint8_t> OledCanvas::generateRawData(const QImage &img) {
    QVector<uint8_t> data;
    data.reserve(1024);

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x += 8) {
            uint8_t byteVal = 0;
            for (int bit = 0; bit < 8; bit++) {
                if (img.pixelColor(x + bit, y) == Qt::white) {
                    byteVal |= (1 << (7 - bit));
                }
            }
            data.push_back(byteVal);
        }
    }
    return data;
}

QImage OledCanvas::getFlattenedFrame(int index) {
    QImage result(128, 64, QImage::Format_ARGB32);
    result.fill(Qt::black);

    if (index >= 0 && index < frames.size()) {
        QPainter painter(&result);
        for (const QImage &layer : frames[index].layers) {
            painter.drawImage(0, 0, layer);
        }
    }
    return result;
}

QVector<uint8_t> OledCanvas::generateCropData(const QImage &img, int &cX, int &cY, int &cW, int &cH) {
    int minX = 128, maxX = -1, minY = 64, maxY = -1;

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x++) {
            if (img.pixelColor(x, y) == Qt::white) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }

    QVector<uint8_t> cropData;
    cX = cY = cW = cH = 0;
    if (maxX < 0) return cropData;

    cX = (minX / 8) * 8;
    maxX = qMin(127, ((maxX / 8) * 8) + 7);
    cW = maxX - cX + 1;
    cY = minY;
    cH = maxY - minY + 1;

    for (int y = cY; y < cY + cH; y++) {
        for (int x = cX; x < cX + cW; x += 8) {
            uint8_t byteVal = 0;
            for (int bit = 0; bit < 8; bit++) {
                if (img.pixelColor(x + bit, y) == Qt::white) {
                    byteVal |= (1 << (7 - bit));
                }
            }
            cropData.push_back(byteVal);
        }
    }
    return cropData;
}

QVector<uint8_t> OledCanvas::generatePixelRleData(const QImage &img) {
    QVector<uint8_t> data;
    bool currentColor = false;
    int count = 0;

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x++) {
            bool isWhite = (img.pixelColor(x, y) == Qt::white);

            if (isWhite == currentColor) {
                if (count == 255) {
                    data.push_back(255);
                    data.push_back(0);
                    count = 1;
                } else {
                    count++;
                }
            } else {
                data.push_back(count);
                currentColor = isWhite;
                count = 1;
            }
        }
    }
    data.push_back(count);
    return data;
}

QVector<uint8_t> OledCanvas::generateByteRleData(const QVector<uint8_t> &rawData) {
    QVector<uint8_t> data;
    if (rawData.isEmpty()) return data;

    uint8_t currentByte = rawData[0];
    int count = 1;

    for (int i = 1; i < rawData.size(); i++) {
        if (rawData[i] == currentByte && count < 255) {
            count++;
        } else {
            data.push_back(count);
            data.push_back(currentByte);
            currentByte = rawData[i];
            count = 1;
        }
    }
    data.push_back(count);
    data.push_back(currentByte);
    return data;
}

QString OledCanvas::formatArrayToCpp(const QVector<uint8_t> &data, const QString &methodName) {
    QString code = QString("// Method: %1\n// Size: %2 bytes\nconst unsigned char optimized_data[] PROGMEM = {\n  ")
    .arg(methodName).arg(data.size());

    for (int i = 0; i < data.size(); i++) {
        code += QString("0x%1, ").arg(data[i], 2, 16, QChar('0'));
        if ((i + 1) % 16 == 0) code += "\n  ";
    }
    return code + "\n};\n";
}

QString OledCanvas::formatArrayCode(const QVector<uint8_t> &data, const QString &methodName, bool isCpp, int frameIndex) {
    QString code;
    QString arrName = (frameIndex == -1) ? "optimized_data" : QString("frame_%1").arg(frameIndex);

    if (isCpp) {
        if (frameIndex == -1) {
            code = QString("// Method: %1\n// Size: %2 bytes\nconst unsigned char %3[] PROGMEM = {\n  ")
            .arg(methodName).arg(data.size()).arg(arrName);
        } else {
            code = QString("// Method: %1 (Frame %2)\n// Size: %3 bytes\nconst unsigned char %4[] PROGMEM = {\n  ")
            .arg(methodName).arg(frameIndex).arg(data.size()).arg(arrName);
        }
    } else {
        if (frameIndex == -1) {
            code = QString("# Method: %1\n# Size: %2 bytes\n%3 = bytearray([\n  ")
            .arg(methodName).arg(data.size()).arg(arrName);
        } else {
            code = QString("# Method: %1 (Frame %2)\n# Size: %3 bytes\n%4 = bytearray([\n  ")
            .arg(methodName).arg(frameIndex).arg(data.size()).arg(arrName);
        }
    }

    for (int i = 0; i < data.size(); i++) {
        code += QString("0x%1, ").arg(data[i], 2, 16, QChar('0'));
        if ((i + 1) % 16 == 0) code += "\n  ";
    }

    return code + (isCpp ? "\n};\n\n" : "\n])\n\n");
}

QString OledCanvas::generateDrawImageCode(int method, bool isCpp) {
    if (isCpp) {
        switch (method) {
        case 1:
            return "void drawImage(const unsigned char* frame_data, int data_size) {\n"
                   "  display.drawBitmap(0, 0, frame_data, 128, 64, WHITE);\n"
                   "}\n";
        case 3:
            return "void drawImage(const unsigned char* frame_data, int data_size) {\n"
                   "  bool isWhite = false;\n"
                   "  int x = 0, y = 0;\n"
                   "  for (int i = 0; i < data_size; i++) {\n"
                   "    uint8_t count = pgm_read_byte(&frame_data[i]);\n"
                   "    for (int p = 0; p < count; p++) {\n"
                   "      if (isWhite) display.drawPixel(x, y, WHITE);\n"
                   "      x++;\n"
                   "      if (x >= 128) { x = 0; y++; }\n"
                   "    }\n"
                   "    isWhite = !isWhite;\n"
                   "  }\n"
                   "}\n";
        case 4:
            return "void drawImage(const unsigned char* frame_data, int data_size) {\n"
                   "  int x = 0, y = 0;\n"
                   "  for (int i = 0; i < data_size; i += 2) {\n"
                   "    uint8_t count = pgm_read_byte(&frame_data[i]);\n"
                   "    uint8_t val = pgm_read_byte(&frame_data[i+1]);\n"
                   "    for (int c = 0; c < count; c++) {\n"
                   "      for (int b = 0; b < 8; b++) {\n"
                   "        if (val & (1 << (7 - b))) display.drawPixel(x + b, y, WHITE);\n"
                   "      }\n"
                   "      x += 8;\n"
                   "      if (x >= 128) { x = 0; y++; }\n"
                   "    }\n"
                   "  }\n"
                   "}\n";
        default: return "";
        }
    } else {
        switch (method) {
        case 1:
            return "import framebuf\n\n"
                   "def draw_image(frame_data):\n"
                   "    fb = framebuf.FrameBuffer(frame_data, 128, 64, framebuf.MONO_HLSB)\n"
                   "    display.blit(fb, 0, 0)\n";
        case 3:
            return "def draw_image(frame_data):\n"
                   "    is_white = False\n"
                   "    x, y = 0, 0\n"
                   "    for count in frame_data:\n"
                   "        for _ in range(count):\n"
                   "            if is_white:\n"
                   "                display.pixel(x, y, 1)\n"
                   "            x += 1\n"
                   "            if x >= 128:\n"
                   "                x, y = 0, y + 1\n"
                   "        is_white = not is_white\n";
        case 4:
            return "def draw_image(frame_data):\n"
                   "    x, y = 0, 0\n"
                   "    for i in range(0, len(frame_data), 2):\n"
                   "        count = frame_data[i]\n"
                   "        val = frame_data[i+1]\n"
                   "        for _ in range(count):\n"
                   "            for b in range(8):\n"
                   "                if val & (1 << (7 - b)):\n"
                   "                    display.pixel(x + b, y, 1)\n"
                   "            x += 8\n"
                   "            if x >= 128:\n"
                   "                x, y = 0, y + 1\n";
        default: return "";
        }
    }
}

QString OledCanvas::generateExportCode(bool optimize, bool isCpp, bool exportAnimation) {
    QString includes, arrayDeclarations, arrayPointers, sizesArray, mainLogic;
    int bestMethod = 1;

    if (!exportAnimation) {
        QImage img = getFlattenedFrame(currentFrameIndex);
        QVector<uint8_t> rawData = generateRawData(img);
        QVector<uint8_t> bestData = rawData;
        QString methodName = "Standard RAW (Unoptimized)";

        if (optimize) {
            methodName = "Standard RAW";
            int minSize = rawData.size();
            QVector<uint8_t> pxRleData = generatePixelRleData(img);
            QVector<uint8_t> byteRleData = generateByteRleData(rawData);

            if (pxRleData.size() < minSize) { minSize = pxRleData.size(); bestMethod = 3; bestData = pxRleData; methodName = "Pixel RLE Compression"; }
            if (byteRleData.size() < minSize) { minSize = byteRleData.size(); bestMethod = 4; bestData = byteRleData; methodName = "Byte RLE Compression"; }
        }

        arrayDeclarations = formatArrayCode(bestData, methodName, isCpp, -1);

        if (isCpp) {
            includes = "#include <Wire.h>\n#include <Adafruit_GFX.h>\n#include <Adafruit_SSD1306.h>\n\n#define SCREEN_WIDTH 128\n#define SCREEN_HEIGHT 64\n#define OLED_RESET -1\nAdafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);\n\n";
            mainLogic = "\nvoid setup() {\n  Wire.begin();\n  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }\n  display.clearDisplay();\n  drawImage(optimized_data, sizeof(optimized_data));\n  display.display();\n}\n\nvoid loop() {\n}\n";
        } else {
            includes = "from machine import Pin, I2C\nimport ssd1306\n\ni2c = I2C(0, scl=Pin(5), sda=Pin(4))\ndisplay = ssd1306.SSD1306_I2C(128, 64, i2c)\n\n";
            mainLogic = "\ndisplay.fill(0)\ndraw_image(optimized_data)\ndisplay.show()\n";
        }

    } else {
        int frameCount = frames.size();
        QString methodName = "Standard RAW (Unoptimized)";

        if (optimize) {
            int totalRaw = 0, totalPxRle = 0, totalByteRle = 0;
            for (int i = 0; i < frameCount; i++) {
                QImage img = getFlattenedFrame(i);
                QVector<uint8_t> raw = generateRawData(img);
                totalRaw += raw.size();
                totalPxRle += generatePixelRleData(img).size();
                totalByteRle += generateByteRleData(raw).size();
            }
            int minSize = totalRaw;
            methodName = "Standard RAW";
            if (totalPxRle < minSize) { minSize = totalPxRle; bestMethod = 3; methodName = "Pixel RLE Compression"; }
            if (totalByteRle < minSize) { minSize = totalByteRle; bestMethod = 4; methodName = "Byte RLE Compression"; }
        }

        QVector<int> frameSizes;
        for (int i = 0; i < frameCount; i++) {
            QImage img = getFlattenedFrame(i);
            QVector<uint8_t> frameData;
            if (bestMethod == 1) frameData = generateRawData(img);
            else if (bestMethod == 3) frameData = generatePixelRleData(img);
            else if (bestMethod == 4) frameData = generateByteRleData(generateRawData(img));

            frameSizes.append(frameData.size());
            arrayDeclarations += formatArrayCode(frameData, methodName, isCpp, i);
        }

        if (isCpp) {
            arrayPointers = "const unsigned char* const frames[] PROGMEM = {\n  ";
            sizesArray = "const int frame_sizes[] = {\n  ";
            for(int i = 0; i < frameCount; i++) {
                arrayPointers += QString("frame_%1").arg(i) + (i == frameCount-1 ? "" : ", ");
                sizesArray += QString::number(frameSizes[i]) + (i == frameCount-1 ? "" : ", ");
            }
            arrayPointers += "\n};\n\n";
            sizesArray += "\n};\n\n";
            includes = "#include <Wire.h>\n#include <Adafruit_GFX.h>\n#include <Adafruit_SSD1306.h>\n\n#define SCREEN_WIDTH 128\n#define SCREEN_HEIGHT 64\n#define OLED_RESET -1\nAdafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);\n\n";
            mainLogic = QString("\nvoid setup() {\n  Wire.begin();\n  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }\n}\n\nvoid loop() {\n  for(int i = 0; i < %1; i++) {\n    display.clearDisplay();\n    const unsigned char* current_frame = (const unsigned char*)pgm_read_ptr(&frames[i]);\n    drawImage(current_frame, frame_sizes[i]);\n    display.display();\n    delay(100);\n  }\n}\n").arg(frameCount);
        } else {
            arrayPointers = "frames = [\n  ";
            for(int i = 0; i < frameCount; i++) arrayPointers += QString("frame_%1").arg(i) + (i == frameCount-1 ? "" : ", ");
            arrayPointers += "\n]\n\n";
            includes = "from machine import Pin, I2C\nimport ssd1306\nimport time\n\ni2c = I2C(0, scl=Pin(5), sda=Pin(4))\ndisplay = ssd1306.SSD1306_I2C(128, 64, i2c)\n\n";
            mainLogic = "\nwhile True:\n    for frame_data in frames:\n        display.fill(0)\n        draw_image(frame_data)\n        display.show()\n        time.sleep(0.1)\n";
        }
    }

    QString drawCode = generateDrawImageCode(bestMethod, isCpp);

    return includes + arrayDeclarations + arrayPointers + sizesArray + drawCode + mainLogic;
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

void OledCanvas::saveToHistory() {
    while (history.size() > historyIndex + 1) {
        history.removeLast();
    }

    HistoryStep step;
    step.frameIndex = currentFrameIndex;
    step.layerIndex = currentLayerIndex;
    step.previousState = tempState;
    step.newState = frames[currentFrameIndex].layers[currentLayerIndex];

    history.append(step);
    historyIndex++;
}

void OledCanvas::undo() {
    if (historyIndex > 0) {
        HistoryStep step = history[historyIndex];

        frames[step.frameIndex].layers[step.layerIndex] = step.previousState;

        currentFrameIndex = step.frameIndex;
        currentLayerIndex = step.layerIndex;

        historyIndex--;
        update();
        emit imageChanged(getFlattenedImage());
        emit layersListChanged();
        emit activeLayerChanged(currentLayerIndex);
    }
}

void OledCanvas::redo() {
    if (historyIndex < history.size() - 1) {
        historyIndex++;
        HistoryStep step = history[historyIndex];

        frames[step.frameIndex].layers[step.layerIndex] = step.newState;

        currentFrameIndex = step.frameIndex;
        currentLayerIndex = step.layerIndex;

        update();
        emit imageChanged(getFlattenedImage());
        emit layersListChanged();
        emit activeLayerChanged(currentLayerIndex);
    }
}

void OledCanvas::clearCanvas() {
    tempState = frames[currentFrameIndex].layers[currentLayerIndex];

    if (currentLayerIndex == 0) {
        frames[currentFrameIndex].layers[currentLayerIndex].fill(Qt::black);
    } else {
        frames[currentFrameIndex].layers[currentLayerIndex].fill(Qt::transparent);
    }

    saveToHistory();

    update();
    emit imageChanged(getFlattenedImage());
}

void OledCanvas::addLayer() {
    QImage newLayer(128, 64, QImage::Format_ARGB32);
    newLayer.fill(Qt::transparent);

    frames[currentFrameIndex].layers.append(newLayer);
    currentLayerIndex = frames[currentFrameIndex].layers.size() - 1;

    saveToHistory();
    update();

    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);
}

void OledCanvas::setCurrentLayer(int index) {
    if (frames.isEmpty() || index < 0 || index >= frames[currentFrameIndex].layers.size()) {
        return;
    }
    currentLayerIndex = index;
    frames[currentFrameIndex].activeLayerIndex = index;

    update();
    emit activeLayerChanged(currentLayerIndex);
}

int OledCanvas::getLayerCount() {
    return frames[currentFrameIndex].layers.size();
}

QImage OledCanvas::getFlattenedImage() {
    QImage result(128, 64, QImage::Format_ARGB32);
    result.fill(Qt::black);

    QPainter painter(&result);
    for (const QImage &layer : frames[currentFrameIndex].layers) {
        painter.drawImage(0, 0, layer);
    }

    return result;
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
        if (frames[currentFrameIndex].layers[currentLayerIndex].pixelColor(p) != targetColor) continue;

        frames[currentFrameIndex].layers[currentLayerIndex].setPixelColor(p, replacementColor);

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

    QPainter p(&frames[currentFrameIndex].layers[currentLayerIndex]);
    p.drawImage(selectionRect.topLeft(), floatingImage);

    isFloating = false;
    hasSelection = false;
    saveToHistory();
    update();
    emit imageChanged(getFlattenedImage());
}

void OledCanvas::copyLayer() {
    if (hasSelection && selectionRect.width() > 0 && selectionRect.height() > 0) {
        internalClipboard = frames[currentFrameIndex].layers[currentLayerIndex].copy(selectionRect);

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

    tempState = frames[currentFrameIndex].layers[currentLayerIndex];
    QPainter p(&frames[currentFrameIndex].layers[currentLayerIndex]);
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

void OledCanvas::addFrame() {
    if (frames.size() >= 16) return;

    Frame newFrame;
    QImage backgroundLayer(128, 64, QImage::Format_ARGB32);
    backgroundLayer.fill(Qt::black);
    newFrame.layers.append(backgroundLayer);

    frames.append(newFrame);
    currentFrameIndex = frames.size() - 1;
    currentLayerIndex = 0;

    saveToHistory();

    emit frameAdded(getFlattenedImage(), currentFrameIndex);
    emit frameChanged(currentFrameIndex);

    update();
}

void OledCanvas::setCurrentFrame(int index) {
    if (index < 0 || index >= frames.size()) {
        return;
    }

    currentFrameIndex = index;
    currentLayerIndex = frames[currentFrameIndex].activeLayerIndex;

    int maxLayer = frames[currentFrameIndex].layers.size() - 1;
    if (currentLayerIndex > maxLayer) {
        currentLayerIndex = maxLayer;
        frames[currentFrameIndex].activeLayerIndex = maxLayer;
    }

    update();
    emit frameChanged(currentFrameIndex);
}

void OledCanvas::togglePlay() {
    if (playTimer->isActive()) {
        playTimer->stop();
        emit isPlayingChanged(false);
    } else {
        if (frames.size() <= 1) return;
        playTimer->start(200);
        emit isPlayingChanged(true);
    }
}

void OledCanvas::onPlayTimerTick() {
    currentFrameIndex++;
    if (currentFrameIndex >= frames.size()) {
        currentFrameIndex = 0;
    }

    update();
    emit frameChanged(currentFrameIndex);
}

void OledCanvas::deleteCurrentFrame() {
    if (frames.size() <= 1) return;

    int indexToDelete = currentFrameIndex;

    frames.removeAt(indexToDelete);

    if (currentFrameIndex >= frames.size()) {
        currentFrameIndex = frames.size() - 1;
    }

    currentLayerIndex = 0;

    saveToHistory();
    update();

    emit frameDeleted(indexToDelete);
    emit frameChanged(currentFrameIndex);
    emit imageChanged(getFlattenedImage());
    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);
}

void OledCanvas::deleteCurrentLayer() {
    QList<QImage>& currentLayers = frames[currentFrameIndex].layers;

    if (currentLayers.size() <= 1) return;

    currentLayers.removeAt(currentLayerIndex);

    if (currentLayerIndex >= currentLayers.size()) {
        currentLayerIndex = currentLayers.size() - 1;
    }

    saveToHistory();
    update();
    emit imageChanged(getFlattenedImage());
    emit layerThumbnailUpdated(currentLayerIndex);
}

int OledCanvas::getCurrentLayerIndex() {
    return currentLayerIndex;
}

int OledCanvas::getFrameCount() {
    return frames.size();
}

int OledCanvas::getCurrentFrameIndex() {
    return currentFrameIndex;
}

QImage OledCanvas::getFrameThumbnail(int index) {
    if (index < 0 || index >= frames.size()) return QImage();

    QImage result(128, 64, QImage::Format_ARGB32);
    result.fill(Qt::black);

    QPainter painter(&result);
    for (const QImage &layer : frames[index].layers) {
        painter.drawImage(0, 0, layer);
    }
    return result;
}

QByteArray OledCanvas::saveProjectData() {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    QList<QList<QImage>> framesData;
    for (const Frame &f : frames) {
        framesData.append(f.layers);
    }

    stream << framesData;
    return data;
}

bool OledCanvas::loadProjectData(const QByteArray &data) {
    QDataStream stream(data);
    QList<QList<QImage>> loadedFramesData;

    stream >> loadedFramesData;

    if (loadedFramesData.isEmpty()) {
        return false;
    }

    frames.clear();
    for (const QList<QImage> &layers : loadedFramesData) {
        Frame f;
        f.layers = layers;
        frames.append(f);
    }

    currentFrameIndex = 0;
    currentLayerIndex = frames[0].layers.size() - 1;

    history.clear();
    historyIndex = -1;
    saveToHistory();

    update();
    emit imageChanged(getFlattenedImage());
    emit frameChanged(currentFrameIndex);

    return true;
}

QImage OledCanvas::getLayerThumbnail(int index) {
    QImage result(128, 64, QImage::Format_ARGB32);
    result.fill(Qt::black); // Глухий чорний фон

    if (frames.isEmpty() || index < 0 || index >= frames[currentFrameIndex].layers.size()) {
        return result;
    }

    QPainter painter(&result);
    painter.drawImage(0, 0, frames[currentFrameIndex].layers[index]);

    return result;
}
