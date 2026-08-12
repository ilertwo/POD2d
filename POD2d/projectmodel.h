// projectmodel.h
#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QObject>
#include <QImage>
#include <QList>

enum class HandleType { None, TopLeft, TopRight, BottomLeft, BottomRight, Move };

struct HistoryStep {
    int frameIndex;
    int layerIndex;
    QImage previousState;
    QImage newState;
};

struct Frame {
    QList<QImage> layers;
    int activeLayerIndex = 0;
};

class ProjectModel : public QObject {
    Q_OBJECT
public:
    explicit ProjectModel(QObject *parent = nullptr);

    // Управління кадрами та шарами
    void addFrame();
    void deleteCurrentFrame();
    void addLayer();
    void deleteCurrentLayer();

    // Робота з поточним зображенням
    QImage& activeImage();
    QImage getFlattenedImage() const;
    QImage getFlattenedFrame(int index) const;
    QImage getFrameThumbnail(int index) const;
    QImage getLayerThumbnail(int index) const;

    // Undo / Redo / Серіалізація
    void saveToHistory();
    void undo();
    void redo();
    QByteArray saveProjectData() const;
    bool loadProjectData(const QByteArray &data);

    int getCurrentFrameIndex();
    int getFrameCount();
    void clearCanvas();
    void setCurrentLayer(int index);
    int getCurrentLayerIndex();
    void onPlayTimerTick();
    void setCurrentFrame(int index);
    void setCanvasImage(QImage image);
    void togglePlay();
    void copyLayer();
    void cutLayer();
    void pasteToLayer();
    void commitImageToCurrentLayer(const QPoint &pos, const QImage &image);
    void setClipboardImage(const QImage &img);
    QImage getClipboardImage() const;
    void clearRectOnCurrentLayer(const QRect &rect);

    QImage& getActiveLayerImage();
    const QList<QImage>& getCurrentLayers() const;
    int getCurrentLayerIndex() const { return currentLayerIndex; }
    void saveHistoryStep();
    void notifyImageChanged();

    void initDefaultProject();
    void syncUIAfterHistoryStep();

    int getLayerCount() const;

    void setCanvasImage(const QImage &image);

signals:
    void frameListChanged();
    void layerListChanged();

    void layersListChanged();
    void layerThumbnailUpdated(int index);
    void activeLayerChanged(int index);

    void imageChanged(const QImage &newImage);
    void frameAdded(const QImage &thumbnail, int index);
    void frameChanged(int index);
    void isPlayingChanged(bool playing);
    void frameDeleted(int deletedIndex);

private:

    Frame createDefaultFrame() const;

private:
    static constexpr int MAX_FRAMES = 16;
    static constexpr int MAX_LAYERS = 16;
    static constexpr int CANVAS_WIDTH = 128;
    static constexpr int CANVAS_HEIGHT = 64;
    static constexpr int PLAYBACK_SPEED_MS = 200;

    QList<Frame> frames;
    QList<HistoryStep> history;
    QList<QImage> layers;

    int currentFrameIndex = 0;
    int currentLayerIndex = 0;

    bool onionSkinEnabled = true;
    QImage canvasImage;
    QImage tempState;
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

#endif // PROJECTMODEL_H
