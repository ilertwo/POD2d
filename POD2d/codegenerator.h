#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <QString>
#include <QVector>
#include <QImage>
#include <QList>

enum class ExportMethod {
    Raw = 1,
    PixelRle = 3,
    ByteRle = 4
};

class CodeGenerator {
public:
    static QString generateExportCode(const QList<QImage>& frames, int currentFrameIndex, bool optimize, bool isCpp, bool exportAnimation);

    static QVector<uint8_t> generateRawData(const QImage &img);
    static QVector<uint8_t> generateCropData(const QImage &img, int &cX, int &cY, int &cW, int &cH);
    static QVector<uint8_t> generatePixelRleData(const QImage &img);
    static QVector<uint8_t> generateByteRleData(const QVector<uint8_t> &rawData);
    static QString formatArrayCode(const QVector<uint8_t> &data, const QString &methodName, bool isCpp, int frameIndex = -1);
    static QString generateDrawImageCode(ExportMethod method, bool isCpp);

private:
    static constexpr int CANVAS_WIDTH = 128;
    static constexpr int CANVAS_HEIGHT = 64;

    static uint8_t extractByte(const QImage &img, int startX, int y);
};

#endif // CODEGENERATOR_H
