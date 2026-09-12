#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <QString>
#include <QVector>
#include <QImage>
#include <QList>
#include <cstdint>

enum class ExportMethod {
    Raw = 1,
    PixelRle = 3,
    ByteRle = 4
};

class CodeGenerator {
public:
    static QString generateExportCode(const QList<QImage>& frames, int currentFrameIndex, bool optimize, bool isCpp, bool exportAnimation, bool isRGB, bool isDataOnly = false, bool invertColors = false);

    static QVector<uint8_t> generateRawData(const QImage &img, bool invertColors = false);
    static QVector<uint8_t> generateCropData(const QImage &img, int &cX, int &cY, int &cW, int &cH);
    static QVector<uint8_t> generatePixelRleData(const QImage &img, bool invertColors = false);
    static QVector<uint8_t> generateByteRleData(const QVector<uint8_t> &rawData);
    static QString formatArrayCode(const QVector<uint8_t> &data, const QString &methodName, bool isCpp, int frameIndex = -1);
    static QString generateDrawImageCode(ExportMethod method, bool isCpp);

    static QVector<uint16_t> generateRawDataRGB(const QImage &img);
    static QString formatArrayCodeRGB(const QVector<uint16_t> &data, const QString &methodName, bool isCpp, int frameIndex = -1);
    static QString generateDrawImageCodeRGB(bool isCpp);

private:
    static constexpr int CANVAS_WIDTH = 128;
    static constexpr int CANVAS_HEIGHT = 64;

    static uint8_t extractByteHorizontal(const QImage &img, int startX, int y, bool invertColors = false);
    static uint8_t extractByteVertical(const QImage &img, int x, int startY, bool invertColors = false);
};

#endif // CODEGENERATOR_H
