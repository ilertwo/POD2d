#include "projectmodel.h"
#include <QPainter>
#include <QImage>
#include <QTimer>
#include <QtGlobal>
#include <QIODevice>
#include <QSettings>

ProjectModel::ProjectModel(QObject *parent)
    : QObject(parent),
    currentFrameIndex(0),
    playTimer(new QTimer(this))
{
    connect(playTimer, &QTimer::timeout, this, &ProjectModel::onPlayTimerTick);

    //initDefaultProject();
}

void ProjectModel::initDefaultProject(int width, int height) {
    frames.clear();
    history.clear();
    historyIndex = -1;

    Frame firstFrame;
    QImage baseLayer(width, height, QImage::Format_ARGB32);
    baseLayer.fill(Qt::black);

    firstFrame.layers.append(baseLayer);
    firstFrame.activeLayerIndex = 0;

    frames.append(firstFrame);

    currentFrameIndex = 0;
    frames[currentFrameIndex].activeLayerIndex = 0;

    loadSettings();
}

void ProjectModel::loadSettings() {
    QSettings settings("POD2d", "EditorSettings");
    int settingsHistorySteps = settings.value("editor/undoLimit", 50).toInt();
    setMaxHistorySteps(settingsHistorySteps);
}

// ==========================================
// 1. FRAME MANAGEMENT
// ==========================================
void ProjectModel::addFrame() {
    if (frames.size() >= MAX_FRAMES) return;

    QList<Frame> backupFrames = frames;
    int backupIndex = currentFrameIndex;

    frames.append(createDefaultFrame());

    currentFrameIndex = frames.size() - 1;
    frames[currentFrameIndex].activeLayerIndex = 0;

    saveStructuralHistoryStep(backupFrames, backupIndex);

    emit framesListChanged();
    emit frameChanged(currentFrameIndex);
    notifyImageChanged();
}

Frame ProjectModel::createDefaultFrame() const {
    Frame frame;
    QImage baseLayer(CANVAS_WIDTH, CANVAS_HEIGHT, QImage::Format_ARGB32);
    baseLayer.fill(Qt::black);

    frame.layers.append(baseLayer);
    frame.activeLayerIndex = 0;

    return frame;
}

void ProjectModel::deleteCurrentFrame() {
    if (frames.size() <= 1) return;

    QList<Frame> backupFrames = frames;
    int backupIndex = currentFrameIndex;

    const int indexToDelete = currentFrameIndex;
    frames.removeAt(indexToDelete);

    if (currentFrameIndex >= frames.size()) {
        currentFrameIndex = frames.size() - 1;
    }

    if (frames[currentFrameIndex].activeLayerIndex >= frames[currentFrameIndex].layers.size()) {
        frames[currentFrameIndex].activeLayerIndex = frames[currentFrameIndex].layers.size() - 1;
    }

    saveStructuralHistoryStep(backupFrames, backupIndex);

    emit framesListChanged();
    emit frameChanged(currentFrameIndex);
    emit imageChanged(getFlattenedImage());
    emit layersListChanged();
    emit activeLayerChanged(getCurrentLayerIndex());
}

void ProjectModel::setCurrentFrame(int index) {
    if (index < 0 || index >= frames.size() || index == currentFrameIndex) {
        return;
    }

    currentFrameIndex = index;
    frames[currentFrameIndex].activeLayerIndex = frames[currentFrameIndex].activeLayerIndex;

    const int maxLayer = frames[currentFrameIndex].layers.size() - 1;
    if (getCurrentLayerIndex() > maxLayer) {
        frames[currentFrameIndex].activeLayerIndex = maxLayer;
        frames[currentFrameIndex].activeLayerIndex = maxLayer;
    }

    emit frameChanged(currentFrameIndex);
    emit imageChanged(getFlattenedImage());
    emit layersListChanged();
    emit activeLayerChanged(getCurrentLayerIndex());
}

int ProjectModel::getFrameCount() const {
    return frames.size();
}

int ProjectModel::getCurrentFrameIndex() const {
    return currentFrameIndex;
}

// ==========================================
// 2. LAYER MANAGEMENT
// ==========================================
void ProjectModel::addLayer() {
    if (frames.isEmpty() || frames[currentFrameIndex].layers.size() >= MAX_LAYERS) return;

    QList<Frame> backupFrames = frames;
    int backupIndex = currentFrameIndex;

    QList<QImage>& currentLayers = frames[currentFrameIndex].layers;

    QImage newLayer(CANVAS_WIDTH, CANVAS_HEIGHT, QImage::Format_ARGB32);
    newLayer.fill(Qt::transparent);

    currentLayers.append(newLayer);
    frames[currentFrameIndex].activeLayerIndex = currentLayers.size() - 1;

    saveStructuralHistoryStep(backupFrames, backupIndex);

    emit layersListChanged();
    emit activeLayerChanged(getCurrentLayerIndex());
    emit imageChanged(getFlattenedImage());
}

void ProjectModel::deleteCurrentLayer() {
    if (frames.isEmpty() || frames[currentFrameIndex].layers.size() <= 1) return;

    QList<Frame> backupFrames = frames;
    int backupIndex = currentFrameIndex;

    QList<QImage>& currentLayers = frames[currentFrameIndex].layers;

    int indexToDelete = frames[currentFrameIndex].activeLayerIndex;

    currentLayers.removeAt(indexToDelete);

    if (frames[currentFrameIndex].activeLayerIndex >= currentLayers.size()) {
        frames[currentFrameIndex].activeLayerIndex = currentLayers.size() - 1;
    }

    saveStructuralHistoryStep(backupFrames, backupIndex);

    emit layersListChanged();
    emit activeLayerChanged(getCurrentLayerIndex());
    emit imageChanged(getFlattenedImage());
}

int ProjectModel::getCurrentLayerIndex() const {
    if (frames.isEmpty()) return 0;
    return frames[currentFrameIndex].activeLayerIndex;
}

void ProjectModel::setCurrentLayer(int index) {
    if (frames.isEmpty() || index < 0 || index >= frames[currentFrameIndex].layers.size() || index == getCurrentLayerIndex()) {
        return;
    }

    frames[currentFrameIndex].activeLayerIndex = index;

    emit activeLayerChanged(index);
}

int ProjectModel::getLayerCount() const {
    if (frames.isEmpty()) return 0;
    return frames[currentFrameIndex].layers.size();
}

// ==========================================
// 3. DATA ACCESS (Images)
// ==========================================
QImage& ProjectModel::getActiveLayerImage() {
    Q_ASSERT_X(!frames.isEmpty(), "getActiveLayerImage", "Critical error: frame array is empty!");

    return frames[currentFrameIndex].layers[getCurrentLayerIndex()];
}

const QList<QImage>& ProjectModel::getCurrentLayers() const {
    Q_ASSERT_X(!frames.isEmpty(), "getCurrentLayers", "Critical error: frame array is empty!");

    return frames[currentFrameIndex].layers;
}

QImage ProjectModel::getFlattenedImage() const {
    QImage result(CANVAS_WIDTH, CANVAS_HEIGHT, QImage::Format_ARGB32);
    result.fill(Qt::black);

    if (!frames.isEmpty()) {
        QPainter painter(&result);
        for (const QImage &layer : frames[currentFrameIndex].layers) {
            painter.drawImage(0, 0, layer);
        }
    }

    return result;
}

QImage ProjectModel::getFlattenedFrame(int index) const {
    QImage result(CANVAS_WIDTH, CANVAS_HEIGHT, QImage::Format_ARGB32);
    result.fill(Qt::black);

    if (index >= 0 && index < frames.size()) {
        QPainter painter(&result);
        for (const QImage &layer : frames[index].layers) {
            painter.drawImage(0, 0, layer);
        }
    }
    return result;
}

QImage ProjectModel::getFrameThumbnail(int index) const {
    return getFlattenedFrame(index);
}

QImage ProjectModel::getLayerThumbnail(int index) const {
    QImage result(CANVAS_WIDTH, CANVAS_HEIGHT, QImage::Format_ARGB32);
    result.fill(Qt::black);

    if (frames.isEmpty() || index < 0 || index >= frames[currentFrameIndex].layers.size()) {
        return result;
    }

    QPainter painter(&result);
    painter.drawImage(0, 0, frames[currentFrameIndex].layers[index]);

    return result;
}

QImage ProjectModel::getCurrentLayerImage() const {
    if (frames.isEmpty()) {
        return QImage();
    }

    int layerIndex = getCurrentLayerIndex();
    if (layerIndex >= 0 && layerIndex < frames[currentFrameIndex].layers.size()) {
        return frames[currentFrameIndex].layers[layerIndex];
    }

    return QImage();
}

// ==========================================
// 4. CLIPBOARD AND EDITING
// ==========================================
void ProjectModel::setCanvasSize(int width, int height) {
    CANVAS_WIDTH = width;
    CANVAS_HEIGHT = height;
}

void ProjectModel::setClipboardImage(const QImage &img) {
    internalClipboard = img;
}

QImage ProjectModel::getClipboardImage() const {
    return internalClipboard;
}

void ProjectModel::commitImageToCurrentLayer(const QPoint &pos, const QImage &image) {
    if (frames.isEmpty() || image.isNull()) return;

    QImage &activeLayer = getActiveLayerImage();
    QImage previousState = activeLayer;
    QPainter p(&activeLayer);
    p.drawImage(pos, image);

    saveHistoryStep(previousState);
    emit imageChanged(getFlattenedImage());
}

void ProjectModel::clearRectOnCurrentLayer(const QRect &rect) {
    if (rect.isEmpty() || frames.isEmpty()) return;

    QImage &activeLayer = getActiveLayerImage();

    QImage previousState = activeLayer;

    QPainter p(&activeLayer);
    p.setCompositionMode(QPainter::CompositionMode_Source);

    const QColor clearColor = (getCurrentLayerIndex() == 0) ? Qt::black : Qt::transparent;
    p.fillRect(rect, clearColor);

    saveHistoryStep(previousState);

    notifyImageChanged();
}

void ProjectModel::clearCanvas() {
    if (frames.isEmpty()) return;

    QImage &activeLayer = getActiveLayerImage();
    QImage previousState = activeLayer;

    if (getCurrentLayerIndex() == 0) {
        activeLayer.fill(Qt::black);
    } else {
        activeLayer.fill(Qt::transparent);
    }

    saveHistoryStep(previousState);
    emit imageChanged(getFlattenedImage());
}

// ==========================================
// 5. UNDO / REDO / SERIALIZATION
// ==========================================
void ProjectModel::setMaxHistorySteps(int limit){
    maxHistorySteps = limit;

    while (history.size() > maxHistorySteps) {
        history.removeFirst();
        historyIndex--;
    }
}

void ProjectModel::saveHistoryStep(const QImage &previousState) {
    QImage currentState = getActiveLayerImage();

    if (previousState == currentState) {
        return;
    }

    if (historyIndex + 1 < history.size()) {
        history.erase(history.begin() + historyIndex + 1, history.end());
    }

    HistoryStep step;
    step.isStructuralChange = false;
    step.frameIndex = currentFrameIndex;
    step.layerIndex = getCurrentLayerIndex();
    step.previousState = previousState;
    step.newState = currentState;

    history.append(step);
    historyIndex++;

    if (history.size() > maxHistorySteps) {
        history.removeFirst();
        historyIndex--;
    }

    emit projectModified();
}

void ProjectModel::saveStructuralHistoryStep(const QList<Frame>& oldFrames, int oldFrameIdx) {
    if (historyIndex + 1 < history.size()) {
        history.erase(history.begin() + historyIndex + 1, history.end());
    }

    HistoryStep step;
    step.isStructuralChange = true;
    step.oldFrames = oldFrames;
    step.newFrames = frames;
    step.oldCurrentFrame = oldFrameIdx;
    step.newCurrentFrame = currentFrameIndex;

    history.append(step);
    historyIndex++;

    if (history.size() > maxHistorySteps) {
        history.removeFirst();
        historyIndex--;
    }

    emit projectModified();
}

void ProjectModel::undo() {
    if (historyIndex < 0) return;

    const HistoryStep &step = history[historyIndex];

    if (step.isStructuralChange) {
        frames = step.oldFrames;
        currentFrameIndex = step.oldCurrentFrame;
    } else {
        frames[step.frameIndex].layers[step.layerIndex] = step.previousState;
        currentFrameIndex = step.frameIndex;
        frames[currentFrameIndex].activeLayerIndex = step.layerIndex;
    }

    historyIndex--;

    syncUIAfterHistoryStep();
}

void ProjectModel::redo() {
    if (historyIndex + 1 >= history.size()) return;

    historyIndex++;
    const HistoryStep &step = history[historyIndex];

    if (step.isStructuralChange) {
        frames = step.newFrames;
        currentFrameIndex = step.newCurrentFrame;
    } else {
        frames[step.frameIndex].layers[step.layerIndex] = step.newState;
        currentFrameIndex = step.frameIndex;
        frames[currentFrameIndex].activeLayerIndex = step.layerIndex;
    }

    syncUIAfterHistoryStep();
}

bool ProjectModel::loadProjectData(const QByteArray &data) {
    QDataStream stream(data);
    QList<QList<QImage>> loadedFramesData;

    stream >> loadedFramesData;

    if (stream.status() != QDataStream::Ok || loadedFramesData.isEmpty()) {
        return false;
    }

    frames.clear();
    frames.reserve(loadedFramesData.size());

    for (const QList<QImage> &layers : loadedFramesData) {
        Frame f;
        f.layers = layers;
        frames.append(f);
    }

    currentFrameIndex = 0;
    frames[currentFrameIndex].activeLayerIndex = frames[0].layers.size() - 1;

    history.clear();
    historyIndex = -1;

    loadSettings();

    emit imageChanged(getFlattenedImage());
    emit frameChanged(currentFrameIndex);
    emit layersListChanged();
    emit activeLayerChanged(getCurrentLayerIndex());

    return true;
}

QByteArray ProjectModel::saveProjectData() const {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    QList<QList<QImage>> framesData;
    framesData.reserve(frames.size());

    for (const Frame &f : frames) {
        framesData.append(f.layers);
    }

    stream << framesData;
    return data;
}

// ==========================================
// 6. ANIMATION AND UI
// ==========================================
void ProjectModel::togglePlay() {
    if (playTimer->isActive()) {
        playTimer->stop();
        emit isPlayingChanged(false);
    } else {
        if (frames.size() <= 1) return;

        playTimer->start(PLAYBACK_SPEED_MS);
        emit isPlayingChanged(true);
    }
}

void ProjectModel::onPlayTimerTick() {
    const int nextFrame = (currentFrameIndex + 1) % frames.size();

    setCurrentFrame(nextFrame);
}

void ProjectModel::notifyImageChanged() {
    emit imageChanged(getFlattenedImage());
    emit layerThumbnailUpdated(getCurrentLayerIndex());
}

void ProjectModel::syncUIAfterHistoryStep() {
    emit framesListChanged();
    emit forceUIFrameSelection(currentFrameIndex);
    emit forceUILayerSelection(getCurrentLayerIndex());
    emit imageChanged(getFlattenedImage());
    emit frameChanged(currentFrameIndex);
    emit activeLayerChanged(getCurrentLayerIndex());
    emit layersListChanged();
}
