#include "oledcanvas.h"

OledCanvas::OledCanvas(QWidget *parent) : QWidget(parent) {
    scaleFactor = 5;

    canvasImage = QImage(128, 64, QImage::Format_RGB32);
    canvasImage.fill(Qt::black);

    setFixedSize(128 * scaleFactor, 64 * scaleFactor);
}

void OledCanvas::clearCanvas() {
    canvasImage.fill(Qt::black);
    update();
}

void OledCanvas::mouseMoveEvent(QMouseEvent *event) {
    int x = event->x() / scaleFactor;
    int y = event->y() / scaleFactor;

    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
        if (event->buttons() & Qt::LeftButton) {
            canvasImage.setPixelColor(x, y, Qt::white);
        } else if (event->buttons() & Qt::RightButton) {
            canvasImage.setPixelColor(x, y, Qt::black);
        }
        update();
    }
}

void OledCanvas::mousePressEvent(QMouseEvent *event) {
    mouseMoveEvent(event);
}

void OledCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    //полотно
    painter.drawImage(QRect(0, 0, 128 * scaleFactor, 64 * scaleFactor), canvasImage);

    painter.setPen(QColor(50, 50, 50));
    for (int i = 0; i <= 128; ++i) {
        painter.drawLine(i * scaleFactor, 0, i * scaleFactor, 64 * scaleFactor);
    }
    for (int i = 0; i <= 64; ++i) {
        painter.drawLine(0, i * scaleFactor, 128 * scaleFactor, i * scaleFactor);
    }
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


