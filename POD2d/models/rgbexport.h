#pragma once

#include <QImage>
#include <QList>
#include <QString>
#include <QVector>

namespace RgbExport {

enum class Method {
    Raw,
    PixelRle,
    PaletteIndexed,
    PaletteRle
};

struct Result {
    QString arrays;
    QString pointers;
    QString sizes;
    QString includes;
    QString mainLogic;
    QString drawCode;
    QString report;
    Method  method = Method::Raw;
};

Result build(const QList<QImage> &frames, int startIndex, int endIndex,
             bool animation, bool optimize, bool invert,
             bool isCpp, const QString &prefix, bool useProgmem);

}
