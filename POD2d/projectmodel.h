#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QObject>
#include <QImage>
#include <QList>
#include <QRect>
#include <QPoint>
#include <QByteArray>



// Structure of a single frame (contains its own layers)
struct Frame {
    QList<QImage> layers;
    int activeLayerIndex = 0;
};

// Undo/Redo history step structure
struct HistoryStep {
    bool isStructuralChange;

    // isStructuralChange = false
    int frameIndex;
    int layerIndex;
    QImage previousState;
    QImage newState;

    // isStructuralChange = true
    QList<Frame> oldFrames;
    QList<Frame> newFrames;
    int oldCurrentFrame;
    int newCurrentFrame;
};

class ProjectModel : public QObject {
    Q_OBJECT
public:
    explicit ProjectModel(QObject *parent = nullptr);
    void initDefaultProject(int width, int height);
    void loadSettings();

    // 1. FRAME MANAGEMENT
    void addFrame();
    void deleteCurrentFrame();
    int getFrameCount() const;
    int getCurrentFrameIndex() const;
    void setCurrentFrame(int index);

    // 2. LAYER MANAGEMENT
    void addLayer();
    void deleteCurrentLayer();
    int getLayerCount() const;
    int getCurrentLayerIndex() const;
    void setCurrentLayer(int index);

    // 3. DATA ACCESS (Images)
    QImage& getActiveLayerImage();
    const QList<QImage>& getCurrentLayers() const;
    QImage getFlattenedImage() const;
    QImage getFlattenedFrame(int index) const;
    QImage getFrameThumbnail(int index) const;
    QImage getLayerThumbnail(int index) const;
    QImage getCurrentLayerImage() const;

    // 4. CLIPBOARD AND EDITING
    void setCanvasSize(int width, int height);
    void setClipboardImage(const QImage &img);
    QImage getClipboardImage() const;
    void commitImageToCurrentLayer(const QPoint &pos, const QImage &image);
    void clearRectOnCurrentLayer(const QRect &rect);
    void clearCanvas();

    // 5. UNDO / REDO / SERIALIZATION
    void setMaxHistorySteps(int limit);
    void saveHistoryStep(const QImage &previousState);
    void saveStructuralHistoryStep(const QList<Frame>& oldFrames, int oldFrameIdx);
    void undo();
    void redo();
    QByteArray saveProjectData() const;
    bool loadProjectData(const QByteArray &data);

    // 6. ANIMATION AND UI
    void togglePlay();
    void onPlayTimerTick();
    void notifyImageChanged();
    void syncUIAfterHistoryStep();

signals:
    // Signals for updating the interface (MainWindow)
    void frameListChanged();
    void layersListChanged();
    void layerThumbnailUpdated(int index);
    void activeLayerChanged(int index);
    void activeFrameChanged(int index);
    void projectModified();

    void imageChanged(const QImage &newImage);
    void frameAdded(const QImage &thumbnail, int index);
    void frameChanged(int index);
    void isPlayingChanged(bool playing);
    void frameDeleted(int deletedIndex);
    void forceUIFrameSelection(int frameIndex);
    void forceUILayerSelection(int layerIndex);
    void framesListChanged();

private:
    Frame createDefaultFrame() const;

    // Configuration constants
    static constexpr int MAX_FRAMES = 16;
    static constexpr int MAX_LAYERS = 16;
    static constexpr int PLAYBACK_SPEED_MS = 200;

    // Width X Height
    int CANVAS_WIDTH = 128;
    int CANVAS_HEIGHT = 64;

    // Single Source of Truth
    QList<Frame> frames;
    int currentFrameIndex = 0;

    // History and clipboard
    QList<HistoryStep> history;
    int maxHistorySteps = 50;
    int historyIndex = -1;
    QImage internalClipboard;

    // Animation
    class QTimer *playTimer;
};

#endif // PROJECTMODEL_H
