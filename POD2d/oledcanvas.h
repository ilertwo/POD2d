#ifndef OLEDCANVAS_H
#define OLEDCANVAS_H

#include <QWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QGraphicsView>
#include <QWheelEvent>
#include <QScrollBar>
#include <QQueue>
#include <QInputDialog>
#include <QLineEdit>
#include <QClipboard>
#include <QDataStream>
#include <QTimer>

struct HistoryStep {
    int frameIndex;
    int layerIndex;
    QImage previousState;
    QImage newState;
};

struct Frame {
    QList<QImage> layers;
};

enum class DrawTool { Pen, Line, Rectangle, Circle, Fill, BrokenLine, Text, Select, Pan };

enum class HandleType { None, TopLeft, TopRight, BottomLeft, BottomRight, Move };

class OledCanvas : public QWidget {
    Q_OBJECT

public:
    explicit OledCanvas(QWidget *parent = nullptr);

    void setTool(DrawTool tool);
    void copyLayer();
    void cutLayer();
    void pasteToLayer();
    void rotateFloatingImage();

    bool loadProjectData(const QByteArray &data);
    QByteArray saveProjectData();

    void clearCanvas();
    void setZoom(double scale);
    void undo();
    void redo();
    void addLayer();
    void setCurrentLayer(int index);
    int getLayerCount();

    void setCanvasImage(QImage image);

    QString generateExportCode(bool optimize, bool isCpp);

    QVector<uint8_t> generateRawData(const QImage &img);
    QVector<uint8_t> generateCropData(const QImage &img, int &cX, int &cY, int &cW, int &cH);
    QVector<uint8_t> generatePixelRleData(const QImage &img);
    QVector<uint8_t> generateByteRleData(const QVector<uint8_t> &rawData);

    QString formatArrayToCpp(const QVector<uint8_t> &data, const QString &methodName);

    void addFrame();
    void setCurrentFrame(int index);
    void togglePlay();

    QImage getFlattenedImage();

    void deleteCurrentFrame();
    void deleteCurrentLayer();

    int getCurrentLayerIndex();

    int getFrameCount();
    int getCurrentFrameIndex();
    QImage getFrameThumbnail(int index);

    QString generateExportCode(bool optimize, bool isCpp, bool exportAnimation = false);
    QString formatArrayCode(const QVector<uint8_t> &data, const QString &methodName, bool isCpp, int frameIndex = -1);
    QImage getFlattenedFrame(int index);
    QString generateDrawImageCode(int method, bool isCpp);
signals:
    void imageChanged(const QImage &newImage);
    void frameAdded(const QImage &thumbnail, int index);
    void frameChanged(int index);
    void isPlayingChanged(bool playing);
    void frameDeleted(int deletedIndex);

protected:
    HandleType getHandleAt(const QPoint& pos);
    void commitFloatingImage();
    void saveToHistory();
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void clampOffset();

    DrawTool currentTool = DrawTool::Pen;
    QPoint startPoint;
    QPoint lastPoint;
    bool isDrawing = false;

    void floodFill(int x, int y, QColor targetColor, QColor replacementColor);

    void onPlayTimerTick();
private:
    QList<Frame> frames;
    int currentFrameIndex = 0;
    int currentLayerIndex = 0;

    bool onionSkinEnabled = true;
    QImage canvasImage;
    double scaleFactor;
    QPointF offset;
    QImage tempState;
    QList<QImage> layers;
    QList<HistoryStep> history;
    int historyIndex = -1;
    QRect selectionRect;
    bool hasSelection = false;
    QImage pastedImage;
    QImage internalClipboard;
    QPoint lastPanPoint;

    QImage floatingImage;
    QImage originalFloatingImage;
    bool isFloating = false;

    HandleType activeHandle = HandleType::None;
    QPoint dragStartMousePos;
    QRect dragStartRect;

    QTimer *playTimer;
};

#endif // OLEDCANVAS_H
