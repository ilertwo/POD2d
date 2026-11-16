#include "rgbexport.h"

#include <QHash>

namespace RgbExport {
namespace {

using Pixels = QVector<uint16_t>;
using Bytes  = QVector<uint8_t>;
Pixels toRgb565(const QImage &src, bool invert) {
    const QImage img = src.convertToFormat(QImage::Format_ARGB32);
    Pixels px;
    px.reserve(img.width() * img.height());
    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
            if (invert) { r = 255 - r; g = 255 - g; b = 255 - b; }
            px.append(uint16_t(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));
        }
    }
    return px;
}

Pixels encodePixelRle(const Pixels &px) {
    Pixels out;
    if (px.isEmpty()) return out;
    out.reserve(px.size());
    uint16_t cur = px[0];
    uint32_t n = 1;
    for (int i = 1; i < px.size(); ++i) {
        if (px[i] == cur && n < 65535) {
            ++n;
        } else {
            out.append(uint16_t(n));
            out.append(cur);
            cur = px[i];
            n = 1;
        }
    }
    out.append(uint16_t(n));
    out.append(cur);
    return out;
}

bool buildPalette(const QList<Pixels> &frames, Pixels &palette, QList<Bytes> &indices) {
    QHash<uint16_t, int> lookup;
    for (const Pixels &f : frames) {
        Bytes idx;
        idx.reserve(f.size());
        for (uint16_t p : f) {
            auto it = lookup.constFind(p);
            if (it == lookup.constEnd()) {
                if (palette.size() >= 256) return false;
                it = lookup.insert(p, palette.size());
                palette.append(p);
            }
            idx.append(uint8_t(it.value()));
        }
        indices.append(idx);
    }
    return true;
}

int bitsPerIndex(int colors) {
    if (colors <= 2)  return 1;
    if (colors <= 4)  return 2;
    if (colors <= 16) return 4;
    return 8;
}

Bytes packIndices(const Bytes &idx, int bpp) {
    Bytes out((idx.size() * bpp + 7) / 8, 0);
    for (int k = 0; k < idx.size(); ++k) {
        const int bit = k * bpp;
        out[bit / 8] |= uint8_t(idx[k] << (8 - bpp - (bit % 8)));
    }
    return out;
}

Bytes encodeIndexRle(const Bytes &idx) {
    Bytes out;
    if (idx.isEmpty()) return out;
    uint8_t cur = idx[0];
    int n = 1;
    for (int i = 1; i < idx.size(); ++i) {
        if (idx[i] == cur && n < 255) {
            ++n;
        } else {
            out.append(uint8_t(n));
            out.append(cur);
            cur = idx[i];
            n = 1;
        }
    }
    out.append(uint8_t(n));
    out.append(cur);
    return out;
}

template <typename T>
int totalBytes(const QList<QVector<T>> &frames) {
    int sum = 0;
    for (const auto &f : frames) sum += f.size() * int(sizeof(T));
    return sum;
}

template <typename T>
QString formatArray(const QVector<T> &data, const QString &cType, const QString &name,
                    const QString &title, bool isCpp, bool progmem) {
    const bool wide = (sizeof(T) == 2);
    const int digits  = wide ? 4 : 2;
    const int perLine = wide ? 8 : 16;
    const int bytes   = data.size() * int(sizeof(T));

    QString code;
    code.reserve(200 + data.size() * (digits + 4));

    if (isCpp) {
        code += "// " + title + "\n";
        code += "// Size: " + QString::number(bytes) + " bytes\n";
        code += "const " + cType + " " + name + "[]" + (progmem ? " PROGMEM" : "") + " = {\n  ";
    } else {
        code += "# " + title + "\n";
        code += "# Size: " + QString::number(bytes) + " bytes\n";
        code += name + (wide ? " = [\n  " : " = bytearray([\n  ");
    }

    for (int i = 0; i < data.size(); ++i) {
        code += "0x" + QString::number(uint(data[i]), 16).rightJustified(digits, '0').toUpper();
        if (i < data.size() - 1) {
            code += ", ";
            if ((i + 1) % perLine == 0) code += "\n  ";
        }
    }

    if (isCpp) code += "\n};\n\n";
    else       code += wide ? "\n]\n\n" : "\n])\n\n";
    return code;
}

QString drawCodeFor(Method m, bool isCpp, int W, int H, const QString &palName, int bpp) {
    if (isCpp) {
        switch (m) {
        case Method::Raw:
            return QString(R"(void drawImage(const uint16_t* frame_data, int data_size) {
  int x = 0, y = 0;
  for (int i = 0; i < data_size; i++) {
    // Safe reading of 16-bit color from PROGMEM for any microcontroller
    uint16_t color = pgm_read_word(&frame_data[i]);
    display.drawPixel(x, y, color);
    x++;
    if (x >= %1) { x = 0; y++; }
  }
}
)").arg(W);

        case Method::PixelRle:
            return QString(R"(// data = pairs: [count, color, count, color, ...]
void drawImage(const uint16_t* frame_data, int data_size) {
  int x = 0, y = 0;
  for (int i = 0; i < data_size; i += 2) {
    uint16_t count = pgm_read_word(&frame_data[i]);
    uint16_t color = pgm_read_word(&frame_data[i + 1]);
    for (uint16_t c = 0; c < count; c++) {
      display.drawPixel(x, y, color);
      x++;
      if (x >= %1) { x = 0; y++; }
    }
  }
}
)").arg(W);

        case Method::PaletteIndexed:
            return QString(R"(// data = palette indices packed %4 bit(s) per pixel, MSB first
void drawImage(const uint8_t* frame_data, int data_size) {
  const uint8_t bpp = %4;
  const uint8_t mask = (1 << bpp) - 1;
  int x = 0, y = 0;
  for (int i = 0; i < data_size && y < %2; i++) {
    uint8_t b = pgm_read_byte(&frame_data[i]);
    for (int s = 8 - bpp; s >= 0 && y < %2; s -= bpp) {
      display.drawPixel(x, y, pgm_read_word(&%3[(b >> s) & mask]));
      x++;
      if (x >= %1) { x = 0; y++; }
    }
  }
}
)").arg(W).arg(H).arg(palName).arg(bpp);

        case Method::PaletteRle:
            return QString(R"(// data = pairs: [count, palette_index, count, palette_index, ...]
void drawImage(const uint8_t* frame_data, int data_size) {
  int x = 0, y = 0;
  for (int i = 0; i < data_size; i += 2) {
    uint8_t count = pgm_read_byte(&frame_data[i]);
    uint16_t color = pgm_read_word(&%2[pgm_read_byte(&frame_data[i + 1])]);
    for (uint8_t c = 0; c < count; c++) {
      display.drawPixel(x, y, color);
      x++;
      if (x >= %1) { x = 0; y++; }
    }
  }
}
)").arg(W).arg(palName);
        }
        return {};
    }

    // ---- MicroPython
    switch (m) {
    case Method::Raw:
        return QString(R"(def draw_image(frame_data):
    # The MicroPython framebuffer supports RGB565
    import struct
    # Converting an int array to bytes for the FrameBuffer
    byte_data = struct.pack('<%dH' % len(frame_data), *frame_data)
    fb = framebuf.FrameBuffer(bytearray(byte_data), %1, %2, framebuf.RGB565)
    display.blit(fb, 0, 0)
)").arg(W).arg(H);

    case Method::PixelRle:
        return QString(R"(# frame_data = [count, color, count, color, ...]
def draw_image(frame_data):
    x, y = 0, 0
    for i in range(0, len(frame_data), 2):
        count = frame_data[i]
        color = frame_data[i + 1]
        for _ in range(count):
            display.pixel(x, y, color)
            x += 1
            if x >= %1:
                x, y = 0, y + 1
)").arg(W);

    case Method::PaletteIndexed:
        return QString(R"(# frame_data = palette indices packed %4 bit(s) per pixel, MSB first
def draw_image(frame_data):
    bpp = %4
    mask = (1 << bpp) - 1
    x, y = 0, 0
    for byte in frame_data:
        for s in range(8 - bpp, -1, -bpp):
            if y >= %2:
                return
            display.pixel(x, y, %3[(byte >> s) & mask])
            x += 1
            if x >= %1:
                x, y = 0, y + 1
)").arg(W).arg(H).arg(palName).arg(bpp);

    case Method::PaletteRle:
        return QString(R"(# frame_data = [count, palette_index, count, palette_index, ...]
def draw_image(frame_data):
    x, y = 0, 0
    for i in range(0, len(frame_data), 2):
        color = %2[frame_data[i + 1]]
        for _ in range(frame_data[i]):
            display.pixel(x, y, color)
            x += 1
            if x >= %1:
                x, y = 0, y + 1
)").arg(W).arg(palName);
    }
    return {};
}

}

Result build(const QList<QImage> &frames, int startIndex, int endIndex,
             bool animation, bool optimize, bool invert,
             bool isCpp, const QString &prefix, bool useProgmem) {
    Result res;
    if (startIndex < 0 || endIndex > frames.size() || startIndex >= endIndex) return res;

    const int frameCount = endIndex - startIndex;
    const int W = frames[startIndex].width();
    const int H = frames[startIndex].height();

    QList<Pixels> raw;
    for (int i = startIndex; i < endIndex; ++i) raw.append(toRgb565(frames[i], invert));

    struct Candidate {
        Method  method;
        QString label;
        bool    ok;
        int     bytes;
    };
    QList<Candidate> cands;
    auto addCandidate = [&cands](Method m, const QString &label, bool ok, int bytes) {
        Candidate c;
        c.method = m;
        c.label  = label;
        c.ok     = ok;
        c.bytes  = bytes;
        cands.append(c);
    };

    addCandidate(Method::Raw, "RAW RGB565 (16-bit)", true, totalBytes(raw));

    QList<Pixels> rleFrames;
    Pixels palette;
    QList<Bytes> idxFrames, packedFrames, palRleFrames;
    int bpp = 8;
    bool paletteOk = false;

    if (optimize) {
        for (const Pixels &f : raw) rleFrames.append(encodePixelRle(f));

        paletteOk = buildPalette(raw, palette, idxFrames);
        if (paletteOk) {
            bpp = bitsPerIndex(palette.size());
            for (const Bytes &idx : idxFrames) {
                packedFrames.append(packIndices(idx, bpp));
                palRleFrames.append(encodeIndexRle(idx));
            }
        }

        const int palBytes = palette.size() * 2;
        addCandidate(Method::PaletteIndexed, QString("Palette %1-bit").arg(bpp),
                     paletteOk, paletteOk ? palBytes + totalBytes(packedFrames) : 0);
        addCandidate(Method::PixelRle, "Pixel RLE (16-bit runs)", true, totalBytes(rleFrames));
        addCandidate(Method::PaletteRle, "Palette + RLE",
                     paletteOk, paletteOk ? palBytes + totalBytes(palRleFrames) : 0);
    }

    int best = 0;
    for (int i = 1; i < cands.size(); ++i)
        if (cands[i].ok && cands[i].bytes < cands[best].bytes) best = i;
    const Method method = cands[best].method;
    res.method = method;

    const QString cm = isCpp ? "//" : "#";
    QString report;
    if (optimize) {
        report += cm + " ===== RGB565 export: size comparison (bytes) =====\n";
        for (int i = 0; i < cands.size(); ++i) {
            const Candidate &c = cands[i];
            report += cm + "   " + c.label.leftJustified(26) + ": ";
            report += c.ok ? QString::number(c.bytes) : QString("n/a (more than 256 colors)");
            if (i == best) report += "   <-- selected";
            report += "\n";
        }
        if (paletteOk)
            report += cm + "   Unique colors: " + QString::number(palette.size()) + "\n";
    }
    if (invert)
        report += cm + " Negative: colors inverted (255 - channel)\n";
    if (!report.isEmpty()) report += "\n";
    res.report = report;

    const bool wide = (method == Method::Raw || method == Method::PixelRle);
    const QString cType = wide ? "uint16_t" : "uint8_t";
    const QString palName = prefix + "palette";

    QString arrays = report;
    if (method == Method::PaletteIndexed || method == Method::PaletteRle) {
        arrays += formatArray(palette, "uint16_t", palName,
                              QString("Palette (%1 colors, RGB565)").arg(palette.size()),
                              isCpp, useProgmem);
    }

    QVector<int> frameSizes;
    for (int k = 0; k < frameCount; ++k) {
        const QString name = prefix + (animation ? QString("frame_%1").arg(k) : QString("color_data"));
        const QString frameStr = animation ? QString(" (Frame %1)").arg(k) : QString();
        const QString title = "Method: " + cands[best].label + frameStr;

        switch (method) {
        case Method::Raw:
            frameSizes.append(raw[k].size());
            arrays += formatArray(raw[k], cType, name, title, isCpp, useProgmem);
            break;
        case Method::PixelRle:
            frameSizes.append(rleFrames[k].size());
            arrays += formatArray(rleFrames[k], cType, name, title, isCpp, useProgmem);
            break;
        case Method::PaletteIndexed:
            frameSizes.append(packedFrames[k].size());
            arrays += formatArray(packedFrames[k], cType, name, title, isCpp, useProgmem);
            break;
        case Method::PaletteRle:
            frameSizes.append(palRleFrames[k].size());
            arrays += formatArray(palRleFrames[k], cType, name, title, isCpp, useProgmem);
            break;
        }
    }
    res.arrays = arrays;

    if (animation) {
        QString list;
        for (int k = 0; k < frameCount; ++k)
            list += prefix + QString("frame_%1").arg(k) + (k == frameCount - 1 ? "" : ", ");

        if (isCpp) {
            res.pointers = "const " + cType + "* const " + prefix + "frames[]"
                         + (useProgmem ? " PROGMEM" : "") + " = {\n  " + list + "\n};\n\n";
            if (method != Method::Raw) {
                QString sz;
                for (int k = 0; k < frameCount; ++k)
                    sz += QString::number(frameSizes[k]) + (k == frameCount - 1 ? "" : ", ");
                res.sizes = "const int " + prefix + "frame_sizes[] = {\n  " + sz + "\n};\n\n";
            }
        } else {
            res.pointers = prefix + "frames = [\n  " + list + "\n]\n\n";
        }
    }

    if (isCpp) {
        res.includes = R"(#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Example for a TFT display (modify as needed)
#include <SPI.h>

#define TFT_CS    10
#define TFT_RST   8
#define TFT_DC    9
Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

)";
        const QString sizeExprAnim   = (method == Method::Raw)
                                     ? QString("%1 * %2").arg(W).arg(H)
                                     : prefix + "frame_sizes[i]";
        const QString single         = prefix + "color_data";
        const QString sizeExprSingle = (method == Method::Raw)
                                     ? QString("%1 * %2").arg(W).arg(H)
                                     : QString("sizeof(%1) / sizeof(%1[0])").arg(single);

        if (animation) {
            res.mainLogic = QString(R"(
void setup() {
  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_BLACK);
}

void loop() {
  for(int i = 0; i < %1; i++) {
    const %2* current_frame = (const %2*)pgm_read_ptr(&%3frames[i]);
    drawImage(current_frame, %4);
    delay(100);
  }
}
)").arg(frameCount).arg(cType, prefix, sizeExprAnim);
        } else {
            res.mainLogic = QString(R"(
void setup() {
  display.initR(INITR_BLACKTAB);
  display.fillScreen(ST77XX_BLACK);
  drawImage(%1, %2);
}

void loop() {}
)").arg(single, sizeExprSingle);
        }
    } else {
        res.includes = "import framebuf\nimport time\n\n";
        res.mainLogic = animation
            ? QString("while True:\n    for f in %1frames:\n        draw_image(f)\n        time.sleep(0.1)\n").arg(prefix)
            : QString("draw_image(%1color_data)\n").arg(prefix);
    }

    res.drawCode = drawCodeFor(method, isCpp, W, H, palName, bpp);
    return res;
}

}
