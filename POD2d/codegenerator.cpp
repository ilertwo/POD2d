#include "codegenerator.h"

QVector<uint8_t> CodeGenerator::generateRawData(const QImage &img) {
    QVector<uint8_t> data;
    // (128 / 8) * 64 = 1024
    data.reserve((CANVAS_WIDTH / 8) * CANVAS_HEIGHT);

    for (int y = 0; y < CANVAS_HEIGHT; ++y) {
        for (int x = 0; x < CANVAS_WIDTH; x += 8) {
            data.append(extractByte(img, x, y));
        }
    }
    return data;
}

QVector<uint8_t> CodeGenerator::generateCropData(const QImage &img, int &cX, int &cY, int &cW, int &cH) {
    int minX = CANVAS_WIDTH, maxX = -1;
    int minY = CANVAS_HEIGHT, maxY = -1;

    for (int y = 0; y < CANVAS_HEIGHT; ++y) {
        for (int x = 0; x < CANVAS_WIDTH; ++x) {
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
    maxX = qMin(CANVAS_WIDTH - 1, ((maxX / 8) * 8) + 7);

    cW = maxX - cX + 1;
    cY = minY;
    cH = maxY - minY + 1;

    cropData.reserve((cW / 8) * cH);

    for (int y = cY; y < cY + cH; ++y) {
        for (int x = cX; x < cX + cW; x += 8) {
            cropData.append(extractByte(img, x, y));
        }
    }

    return cropData;
}

QVector<uint8_t> CodeGenerator::generatePixelRleData(const QImage &img) {
    QVector<uint8_t> data;

    data.reserve(CANVAS_WIDTH * CANVAS_HEIGHT);

    bool currentColor = false;
    int count = 0;

    for (int y = 0; y < CANVAS_HEIGHT; ++y) {
        for (int x = 0; x < CANVAS_WIDTH; ++x) {
            const bool isWhite = (img.pixelColor(x, y) == Qt::white);

            if (isWhite == currentColor) {
                if (count == 255) {
                    data.append(255);
                    data.append(0);
                    count = 1;
                } else {
                    count++;
                }
            } else {
                data.append(count);
                currentColor = isWhite;
                count = 1;
            }
        }
    }

    data.append(count);
    data.squeeze();

    return data;
}

QVector<uint8_t> CodeGenerator::generateByteRleData(const QVector<uint8_t> &rawData) {
    QVector<uint8_t> data;
    if (rawData.isEmpty()) return data;

    data.reserve(rawData.size() * 2);

    uint8_t currentByte = rawData[0];
    int count = 1;

    for (int i = 1; i < rawData.size(); ++i) {
        const uint8_t nextByte = rawData[i];

        if (nextByte == currentByte && count < 255) {
            count++;
        } else {
            data.append(count);
            data.append(currentByte);

            currentByte = nextByte;
            count = 1;
        }
    }

    data.append(count);
    data.append(currentByte);

    data.squeeze();

    return data;
}

QString CodeGenerator::formatArrayCode(const QVector<uint8_t> &data, const QString &methodName, bool isCpp, int frameIndex) {
    QString code;

    code.reserve(200 + data.size() * 6);

    const QString arrName = (frameIndex == -1) ? "optimized_data" : QString("frame_%1").arg(frameIndex);
    const QString frameStr = (frameIndex == -1) ? "" : QString(" (Frame %1)").arg(frameIndex);

    if (isCpp) {
        code += QString("// Method: %1%2\n// Size: %3 bytes\nconst unsigned char %4[] PROGMEM = {\n  ")
                    .arg(methodName, frameStr, QString::number(data.size()), arrName);
    } else {
        code += QString("# Method: %1%2\n# Size: %3 bytes\n%4 = bytearray([\n  ")
        .arg(methodName, frameStr, QString::number(data.size()), arrName);
    }

    for (int i = 0; i < data.size(); ++i) {
        code += QString("0x%1").arg(data[i], 2, 16, QChar('0'));

        if (i < data.size() - 1) {
            code += ", ";
            if ((i + 1) % 16 == 0) {
                code += "\n  ";
            }
        }
    }

    code += (isCpp ? "\n};\n\n" : "\n])\n\n");

    return code;
}

QString CodeGenerator::generateDrawImageCode(ExportMethod method, bool isCpp) {
    if (isCpp) {
        switch (method) {
        case ExportMethod::Raw: // Raw
            return QString(R"(void drawImage(const unsigned char* frame_data, int data_size) {
  display.drawBitmap(0, 0, frame_data, %1, %2, WHITE);
}
)").arg(CANVAS_WIDTH).arg(CANVAS_HEIGHT);

        case ExportMethod::PixelRle: // Pixel RLE
            return QString(R"(void drawImage(const unsigned char* frame_data, int data_size) {
  bool isWhite = false;
  int x = 0, y = 0;
  for (int i = 0; i < data_size; i++) {
    uint8_t count = pgm_read_byte(&frame_data[i]);
    for (int p = 0; p < count; p++) {
      if (isWhite) display.drawPixel(x, y, WHITE);
      x++;
      if (x >= %1) { x = 0; y++; }
    }
    isWhite = !isWhite;
  }
}
)").arg(CANVAS_WIDTH);

        case ExportMethod::ByteRle: // Byte RLE
            return QString(R"(void drawImage(const unsigned char* frame_data, int data_size) {
  int x = 0, y = 0;
  for (int i = 0; i < data_size; i += 2) {
    uint8_t count = pgm_read_byte(&frame_data[i]);
    uint8_t val = pgm_read_byte(&frame_data[i+1]);
    for (int c = 0; c < count; c++) {
      for (int b = 0; b < 8; b++) {
        if (val & (1 << (7 - b))) display.drawPixel(x + b, y, WHITE);
      }
      x += 8;
      if (x >= %1) { x = 0; y++; }
    }
  }
}
)").arg(CANVAS_WIDTH);

        default: return "";
        }
    } else {
        // Python
        switch (method) {
        case ExportMethod::Raw: // Raw
            return QString(R"(import framebuf

def draw_image(frame_data):
    fb = framebuf.FrameBuffer(frame_data, %1, %2, framebuf.MONO_HLSB)
    display.blit(fb, 0, 0)
)").arg(CANVAS_WIDTH).arg(CANVAS_HEIGHT);

        case ExportMethod::PixelRle: // Pixel RLE
            return QString(R"(def draw_image(frame_data):
    is_white = False
    x, y = 0, 0
    for count in frame_data:
        for _ in range(count):
            if is_white:
                display.pixel(x, y, 1)
            x += 1
            if x >= %1:
                x, y = 0, y + 1
        is_white = not is_white
)").arg(CANVAS_WIDTH);

        case ExportMethod::ByteRle: // Byte RLE
            return QString(R"(def draw_image(frame_data):
    x, y = 0, 0
    for i in range(0, len(frame_data), 2):
        count = frame_data[i]
        val = frame_data[i+1]
        for _ in range(count):
            for b in range(8):
                if val & (1 << (7 - b)):
                    display.pixel(x + b, y, 1)
            x += 8
            if x >= %1:
                x, y = 0, y + 1
)").arg(CANVAS_WIDTH);

        default: return "";
        }
    }
}

QString CodeGenerator::generateExportCode(const QList<QImage>& frames, int currentFrameIndex, bool optimize, bool isCpp, bool exportAnimation) {
    ExportMethod bestMethod = ExportMethod::Raw;
    QString methodName = "Standard RAW (Unoptimized)";

    QList<QVector<uint8_t>> finalFramesData;

    const int frameCount = exportAnimation ? frames.size() : 1;
    const int startIndex = exportAnimation ? 0 : currentFrameIndex;
    const int endIndex = exportAnimation ? frames.size() : currentFrameIndex + 1;

    if (!optimize) {
        for (int i = startIndex; i < endIndex; ++i) {
            finalFramesData.append(generateRawData(frames[i]));
        }
    } else {
        int totalRaw = 0, totalPxRle = 0, totalByteRle = 0;
        QList<QVector<uint8_t>> rawList, pxList, byteList;

        for (int i = startIndex; i < endIndex; ++i) {
            QVector<uint8_t> raw = generateRawData(frames[i]);
            QVector<uint8_t> px = generatePixelRleData(frames[i]);
            QVector<uint8_t> byte = generateByteRleData(raw);

            totalRaw += raw.size();
            totalPxRle += px.size();
            totalByteRle += byte.size();

            rawList.append(raw);
            pxList.append(px);
            byteList.append(byte);
        }

        if (totalPxRle < totalRaw && totalPxRle <= totalByteRle) {
            bestMethod = ExportMethod::PixelRle;
            methodName = "Pixel RLE Compression";
            finalFramesData = pxList;
        } else if (totalByteRle < totalRaw && totalByteRle < totalPxRle) {
            bestMethod = ExportMethod::ByteRle;
            methodName = "Byte RLE Compression";
            finalFramesData = byteList;
        } else {
            bestMethod = ExportMethod::Raw;
            methodName = "Standard RAW";
            finalFramesData = rawList;
        }
    }

    QString arrayDeclarations;
    QVector<int> frameSizes;

    for (int i = 0; i < finalFramesData.size(); ++i) {
        frameSizes.append(finalFramesData[i].size());
        arrayDeclarations += formatArrayCode(finalFramesData[i], methodName, isCpp, exportAnimation ? i : -1);
    }

    QString arrayPointers, sizesArray, includes, mainLogic;

    if (exportAnimation) {
        if (isCpp) {
            arrayPointers = "const unsigned char* const frames[] PROGMEM = {\n  ";
            sizesArray = "const int frame_sizes[] = {\n  ";

            for(int i = 0; i < frameCount; ++i) {
                arrayPointers += QString("frame_%1%2").arg(i).arg(i == frameCount - 1 ? "" : ", ");
                sizesArray += QString::number(frameSizes[i]) + (i == frameCount - 1 ? "" : ", ");
            }
            arrayPointers += "\n};\n\n";
            sizesArray += "\n};\n\n";
        } else {
            arrayPointers = "frames = [\n  ";
            for(int i = 0; i < frameCount; ++i) {
                arrayPointers += QString("frame_%1%2").arg(i).arg(i == frameCount - 1 ? "" : ", ");
            }
            arrayPointers += "\n]\n\n";
        }
    }

    if (isCpp) {
        includes = R"(#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

)";
        if (exportAnimation) {
            mainLogic = QString(R"(
void setup() {
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
}

void loop() {
  for(int i = 0; i < %1; i++) {
    display.clearDisplay();
    const unsigned char* current_frame = (const unsigned char*)pgm_read_ptr(&frames[i]);
    drawImage(current_frame, frame_sizes[i]);
    display.display();
    delay(100);
  }
}
)").arg(frameCount);
        } else {
            mainLogic = R"(
void setup() {
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.clearDisplay();
  drawImage(optimized_data, sizeof(optimized_data));
  display.display();
}

void loop() {
}
)";
        }
    } else {
        if (exportAnimation) {
            includes = R"(from machine import Pin, I2C
import ssd1306
import time

i2c = I2C(0, scl=Pin(5), sda=Pin(4))
display = ssd1306.SSD1306_I2C(128, 64, i2c)

)";
            mainLogic = R"(
while True:
    for frame_data in frames:
        display.fill(0)
        draw_image(frame_data)
        display.show()
        time.sleep(0.1)
)";
        } else {
            includes = R"(from machine import Pin, I2C
import ssd1306

i2c = I2C(0, scl=Pin(5), sda=Pin(4))
display = ssd1306.SSD1306_I2C(128, 64, i2c)

)";
            mainLogic = R"(
display.fill(0)
draw_image(optimized_data)
display.show()
)";
        }
    }

    const QString drawCode = generateDrawImageCode(bestMethod, isCpp);

    return includes + arrayDeclarations + arrayPointers + sizesArray + drawCode + mainLogic;
}

uint8_t CodeGenerator::extractByte(const QImage &img, int startX, int y) {
    uint8_t byteVal = 0;
    for (int bit = 0; bit < 8; ++bit) {
        if (img.pixelColor(startX + bit, y) == Qt::white) {
            byteVal |= (1 << (7 - bit));
        }
    }
    return byteVal;
}
