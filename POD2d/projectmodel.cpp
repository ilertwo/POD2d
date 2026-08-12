#include "projectmodel.h"
#include <QPainter>
#include <QImage>
#include <QTimer>
#include <QtGlobal>
#include <QIODevice>

ProjectModel::ProjectModel(QObject *parent)
    : QObject(parent),
    currentFrameIndex(0),
    currentLayerIndex(0),
    playTimer(new QTimer(this))
{
    connect(playTimer, &QTimer::timeout, this, &ProjectModel::onPlayTimerTick);

    initDefaultProject();
}

void ProjectModel::initDefaultProject() {
    frames.clear();
    history.clear();
    historyIndex = -1;

    Frame firstFrame;
    QImage baseLayer(128, 64, QImage::Format_ARGB32);
    baseLayer.fill(Qt::black);

    firstFrame.layers.append(baseLayer);
    firstFrame.activeLayerIndex = 0;

    frames.append(firstFrame);

    currentFrameIndex = 0;
    currentLayerIndex = 0;

    tempState = baseLayer;
}

void ProjectModel::saveToHistory() {
    if (tempState == frames[currentFrameIndex].layers[currentLayerIndex])
        return;

    if (historyIndex + 1 < history.size())
        history.erase(history.begin() + historyIndex + 1, history.end());

    HistoryStep step;
    step.frameIndex = currentFrameIndex;
    step.layerIndex = currentLayerIndex;
    step.previousState = tempState;
    step.newState = frames[currentFrameIndex].layers[currentLayerIndex];

    history.append(step);
    historyIndex++;

    const int MAX_HISTORY_STEPS = 50;
    if (history.size() > MAX_HISTORY_STEPS) {
        history.removeFirst();
        historyIndex--;
    }
}

void ProjectModel::undo() {
    if (historyIndex <= 0) return;

    const HistoryStep &step = history[historyIndex];

    frames[step.frameIndex].layers[step.layerIndex] = step.previousState;

    currentFrameIndex = step.frameIndex;
    currentLayerIndex = step.layerIndex;

    historyIndex--;

    syncUIAfterHistoryStep();
}

void ProjectModel::redo() {
    if (historyIndex >= history.size() - 1) return;

    historyIndex++;
    const HistoryStep &step = history[historyIndex];

    frames[step.frameIndex].layers[step.layerIndex] = step.newState;

    currentFrameIndex = step.frameIndex;
    currentLayerIndex = step.layerIndex;

    syncUIAfterHistoryStep();
}

void ProjectModel::syncUIAfterHistoryStep() {
    emit imageChanged(getFlattenedImage());
    emit frameChanged(currentFrameIndex);
    emit activeLayerChanged(currentLayerIndex);
    emit layersListChanged();
}

void ProjectModel::addFrame() {
    if (frames.size() >= MAX_FRAMES) return;

    frames.append(createDefaultFrame());

    currentFrameIndex = frames.size() - 1;
    currentLayerIndex = 0;

    tempState = frames[currentFrameIndex].layers[currentLayerIndex];

    // TODO: Піксельний saveToHistory() тут не підходить.
    // Щоб скасувати створення кадру, потрібно реалізувати збереження структурних змін.
    // Тому поки що ми не зберігаємо цю дію в поточний стек історії.

    emit frameAdded(getFlattenedImage(), currentFrameIndex);
    emit frameChanged(currentFrameIndex);
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

    const int indexToDelete = currentFrameIndex;

    frames.removeAt(indexToDelete);

    if (currentFrameIndex >= frames.size()) {
        currentFrameIndex = frames.size() - 1;
    }

    currentLayerIndex = 0;

    tempState = frames[currentFrameIndex].layers[currentLayerIndex];

    // TODO: saveToHistory() тут видалено, оскільки піксельна історія не вміє
    // відновлювати видалені кадри. Це потребує окремої логіки збереження структури проєкту.

    emit frameDeleted(indexToDelete);
    emit frameChanged(currentFrameIndex);
    emit imageChanged(getFlattenedImage());
    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);
}

void ProjectModel::addLayer() {
    if (frames.isEmpty()) return;

    QList<QImage>& currentLayers = frames[currentFrameIndex].layers;

    if (currentLayers.size() >= MAX_LAYERS) return;

    QImage newLayer(CANVAS_WIDTH, CANVAS_HEIGHT, QImage::Format_ARGB32);
    newLayer.fill(Qt::transparent);

    currentLayers.append(newLayer);
    currentLayerIndex = currentLayers.size() - 1;

    tempState = currentLayers[currentLayerIndex];

    // TODO: Піксельна історія тут не працює. Видаляємо тимчасово saveToHistory().

    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);
    emit imageChanged(getFlattenedImage());
}

void ProjectModel::deleteCurrentLayer() {
    QList<QImage>& currentLayers = frames[currentFrameIndex].layers;

    if (currentLayers.size() <= 1) return;

    currentLayers.removeAt(currentLayerIndex);

    if (currentLayerIndex >= currentLayers.size()) {
        currentLayerIndex = currentLayers.size() - 1;
    }

    tempState = currentLayers[currentLayerIndex];

    // TODO: Піксельна історія тут не працює. Видаляємо тимчасово saveToHistory().

    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);
    emit imageChanged(getFlattenedImage());
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
    currentLayerIndex = frames[0].layers.size() - 1;

    history.clear();
    historyIndex = -1;

    tempState = frames[currentFrameIndex].layers[currentLayerIndex];

    emit imageChanged(getFlattenedImage());
    emit frameChanged(currentFrameIndex);
    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);

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

void ProjectModel::setCanvasImage(const QImage &image) {
    canvasImage = image;
    emit imageChanged(getFlattenedImage());
}

void ProjectModel::setCurrentFrame(int index) {
    if (index < 0 || index >= frames.size() || index == currentFrameIndex) {
        return;
    }

    currentFrameIndex = index;
    currentLayerIndex = frames[currentFrameIndex].activeLayerIndex;

    const int maxLayer = frames[currentFrameIndex].layers.size() - 1;
    if (currentLayerIndex > maxLayer) {
        currentLayerIndex = maxLayer;
        frames[currentFrameIndex].activeLayerIndex = maxLayer;
    }

    tempState = frames[currentFrameIndex].layers[currentLayerIndex];

    emit frameChanged(currentFrameIndex);
    emit imageChanged(getFlattenedImage());
    emit layersListChanged();
    emit activeLayerChanged(currentLayerIndex);
}

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

int ProjectModel::getCurrentLayerIndex() {
    return currentLayerIndex;
}

int ProjectModel::getFrameCount() {
    return frames.size();
}

int ProjectModel::getCurrentFrameIndex() {
    return currentFrameIndex;
}

void ProjectModel::setCurrentLayer(int index) {
    if (frames.isEmpty() || index < 0 || index >= frames[currentFrameIndex].layers.size() || index == currentLayerIndex) {
        return;
    }

    currentLayerIndex = index;
    frames[currentFrameIndex].activeLayerIndex = index;

    tempState = frames[currentFrameIndex].layers[currentLayerIndex];

    emit activeLayerChanged(currentLayerIndex);
}

int ProjectModel::getLayerCount() const {
    if (frames.isEmpty()) return 0;
    return frames[currentFrameIndex].layers.size();
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

void ProjectModel::clearCanvas() {
    if (frames.isEmpty()) return;

    QImage &activeLayer = frames[currentFrameIndex].layers[currentLayerIndex];

    tempState = activeLayer;

    if (currentLayerIndex == 0) {
        activeLayer.fill(Qt::black);
    } else {
        activeLayer.fill(Qt::transparent);
    }

    saveToHistory();

    emit imageChanged(getFlattenedImage());
}

void ProjectModel::commitImageToCurrentLayer(const QPoint &pos, const QImage &image) {
    if (frames.isEmpty() || image.isNull()) return;

    QImage &activeLayer = frames[currentFrameIndex].layers[currentLayerIndex];

    tempState = activeLayer;

    QPainter p(&activeLayer);
    p.drawImage(pos, image);

    saveToHistory();
    emit imageChanged(getFlattenedImage());
}

QImage& ProjectModel::getActiveLayerImage() {
    Q_ASSERT_X(!frames.isEmpty(), "getActiveLayerImage", "Critical error: frame array is empty!");

    return frames[currentFrameIndex].layers[currentLayerIndex];
}

const QList<QImage>& ProjectModel::getCurrentLayers() const {
    Q_ASSERT_X(!frames.isEmpty(), "getCurrentLayers", "Critical error: frame array is empty!");

    return frames[currentFrameIndex].layers;
}

// зробити saveToHistory() публічним і видалити цей метод.
void ProjectModel::saveHistoryStep() {
    saveToHistory();
}

void ProjectModel::notifyImageChanged() {
    emit imageChanged(getFlattenedImage());
}

void ProjectModel::setClipboardImage(const QImage &img) {
    internalClipboard = img;
}

QImage ProjectModel::getClipboardImage() const {
    return internalClipboard;
}

void ProjectModel::clearRectOnCurrentLayer(const QRect &rect) {
    if (rect.isEmpty() || frames.isEmpty()) return;

    QImage &activeLayer = frames[currentFrameIndex].layers[currentLayerIndex];

    tempState = activeLayer;

    QPainter p(&activeLayer);
    p.setCompositionMode(QPainter::CompositionMode_Source);

    const QColor clearColor = (currentLayerIndex == 0) ? Qt::black : Qt::transparent;
    p.fillRect(rect, clearColor);

    saveToHistory();
    emit imageChanged(getFlattenedImage());
}
