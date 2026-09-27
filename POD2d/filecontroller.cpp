#include "filecontroller.h"
#include "projectmodel.h"
#include "createprojectdialog.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QRegularExpression>
#include <QPainter>
#include <QSet>

FileController::FileController(ProjectModel *model, QWidget *parentWindow, QObject *parent)
    : QObject(parent), m_model(model), m_parentWindow(parentWindow) {}

void FileController::resetState() {
    m_currentFilePath.clear();
    m_currentProjectName.clear();
    m_isModified = false;
    emit stateChanged();
}

void FileController::markModified() {
    if (!m_isModified) {
        m_isModified = true;
        emit stateChanged();
    }
}

void FileController::createProject() {
    CreateProjectDialog dialog(m_parentWindow);
    if (dialog.exec() == QDialog::Accepted) {
        QString newPath = dialog.getFullFilePath();
        if (newPath.isEmpty()) return;

        m_currentFilePath = newPath;
        m_currentProjectName = dialog.getProjectName();
        m_projectWidth = dialog.getWidth();
        m_projectHeight = dialog.getHeight();
        bool isRGB = dialog.isRGBMode();

        if (m_model) {
            m_model->initDefaultProject(m_projectWidth, m_projectHeight, isRGB);
            m_model->setCanvasSize(m_projectWidth, m_projectHeight);
            m_model->notifyImageChanged();
        }

        m_isModified = false;
        saveProject(); // Зберігає та додає в Recent

        emit projectReady(m_projectWidth, m_projectHeight, isRGB);
        emit stateChanged();
    }
}

void FileController::openProject() {
    QSettings settings("POD2d", "EditorSettings");
    QString lastDir = settings.value("lastDirectory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();

    const QString path = QFileDialog::getOpenFileName(
        m_parentWindow, "Open project", lastDir,
        "All Supported Files (*.pod2d *.png);;Pod2D Project (*.pod2d);;PNG Image (*.png)"
        );

    if (path.isEmpty()) return;
    settings.setValue("lastDirectory", QFileInfo(path).absolutePath());

    if (path.endsWith(".png", Qt::CaseInsensitive)) {
        openPngAsProject(path);
    } else {
        loadProjectFromFile(path);
    }
}

void FileController::loadProjectFromFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(m_parentWindow, "Error", "Failed to open the file!");
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    if (!m_model->loadProjectData(data)) {
        QMessageBox::warning(m_parentWindow, "Error", "The project file is corrupted or has an invalid format!");
        return;
    }

    m_currentFilePath = path;
    m_currentProjectName = QFileInfo(path).baseName();

    QImage loadedImg = m_model->getActiveLayerImage();
    m_projectWidth = loadedImg.width();
    m_projectHeight = loadedImg.height();

    m_model->setCanvasSize(m_projectWidth, m_projectHeight);
    m_model->notifyImageChanged();

    m_isModified = false;

    emit recentProjectAdded(path);
    emit projectReady(m_projectWidth, m_projectHeight, m_model->getIsRGB());
    emit stateChanged();
}

void FileController::saveProjectAs() {
    QSettings settings("POD2d", "EditorSettings");
    QString lastDir = settings.value("lastDirectory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();

    QString defaultFileName = m_currentProjectName.isEmpty() ? "Untitled.pod2d" : m_currentProjectName + ".pod2d";
    QString fullPath = lastDir + "/" + defaultFileName;

    QString filePath = QFileDialog::getSaveFileName(
        m_parentWindow, "Save project as...", fullPath,
        "POD2d Project (*.pod2d);;All Files (*)"
        );

    if (filePath.isEmpty()) return;
    settings.setValue("lastDirectory", QFileInfo(filePath).absolutePath());

    m_currentFilePath = filePath;
    m_currentProjectName = QFileInfo(filePath).baseName();

    saveProject();
}

void FileController::saveProject() {
    if (m_currentFilePath.isEmpty()) {
        saveProjectAs();
        return;
    }

    QByteArray projectData = m_model->saveProjectData();
    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(m_parentWindow, "Error", "Failed to save the file. Check access permissions.");
        return;
    }

    file.write(projectData);
    file.close();

    m_isModified = false;
    emit recentProjectAdded(m_currentFilePath);
    emit projectSaved();
    emit stateChanged();
}

void FileController::openPngAsProject(const QString &path) {
    QImage img;
    if (!img.load(path)) {
        QMessageBox::warning(m_parentWindow, "Error", "Failed to load the image!");
        return;
    }

    img = img.convertToFormat(QImage::Format_ARGB32);
    bool isRgbMode = true;
    QSet<QRgb> uniqueColors;

    for (int y = 0; y < img.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            uniqueColors.insert(line[x]);
            if (uniqueColors.size() > 2) break;
        }
        if (uniqueColors.size() > 2) break;
    }
    if (uniqueColors.size() <= 2) isRgbMode = false;

    m_currentFilePath = path;
    m_currentProjectName = QFileInfo(path).baseName();
    m_projectWidth = img.width();
    m_projectHeight = img.height();

    if (m_model) {
        m_model->initDefaultProject(m_projectWidth, m_projectHeight, isRgbMode);
        QImage &firstLayer = m_model->getActiveLayerImage();
        QPainter p(&firstLayer);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(0, 0, img);
        p.end();

        m_model->setCanvasSize(m_projectWidth, m_projectHeight);
        m_model->notifyImageChanged();
    }

    m_isModified = false;
    emit recentProjectAdded(path);
    emit projectReady(m_projectWidth, m_projectHeight, isRgbMode);
    emit stateChanged();
}

void FileController::actionImportPng() {
    QSettings settings("POD2d", "EditorSettings");
    QString lastDir = settings.value("lastDirectory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();
    QString path = QFileDialog::getOpenFileName(
        m_parentWindow, "Import PNG", lastDir, "Images (*.png *.jpg *.bmp)"
        );
    if (path.isEmpty()) return;
    settings.setValue("lastDirectory", QFileInfo(path).absolutePath());
    importPngToCanvas(path);
}

void FileController::importPngToCanvas(const QString &path) {
    if (path.isEmpty()) return;
    QImage img;
    if (!img.load(path)) {
        QMessageBox::warning(m_parentWindow, "Error", "Failed to load the image!");
        return;
    }

    img = img.convertToFormat(QImage::Format_ARGB32);

    if (m_model && !m_model->getIsRGB()) {
        QMap<QRgb, int> colorCounts;
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QRgb rgb = img.pixel(x, y);
                if (qAlpha(rgb) > 128) colorCounts[rgb]++;
            }
        }

        if (colorCounts.size() > 2) {
            QList<QRgb> keys = colorCounts.keys();
            std::sort(keys.begin(), keys.end(), [&colorCounts](QRgb a, QRgb b) {
                return colorCounts[a] > colorCounts[b];
            });

            QRgb dominantColor = keys.value(0, qRgb(0,0,0));
            QRgb secondaryColor = keys.value(1, qRgb(255,255,255));

            for (int y = 0; y < img.height(); ++y) {
                for (int x = 0; x < img.width(); ++x) {
                    QRgb rgb = img.pixel(x, y);
                    if (qAlpha(rgb) <= 128) {
                        img.setPixel(x, y, qRgba(0, 0, 0, 0));
                    } else {
                        int distDom = qAbs(qRed(rgb)-qRed(dominantColor)) + qAbs(qGreen(rgb)-qGreen(dominantColor)) + qAbs(qBlue(rgb)-qBlue(dominantColor));
                        int distSec = qAbs(qRed(rgb)-qRed(secondaryColor)) + qAbs(qGreen(rgb)-qGreen(secondaryColor)) + qAbs(qBlue(rgb)-qBlue(secondaryColor));
                        img.setPixel(x, y, (distDom < distSec) ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
                    }
                }
            }
        }
    }

    if (m_model) {
        m_model->setClipboardImage(img);
        emit requestPasteToLayer();
    }
}

void FileController::actionImportCArray() {
    bool ok;
    QString codeText = QInputDialog::getMultiLineText(m_parentWindow, "Import C-Array",
                                                      "Insert your array (e.g., 0xFF, 0x00...):", "", &ok);
    if (!ok || codeText.isEmpty()) return;

    QSettings settings("POD2d", "EditorSettings");
    bool isU8g2 = (settings.value("export/byteFormat", 0).toInt() == 1);

    QImage img(m_projectWidth, m_projectHeight, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRegularExpression hexRegex("0x[0-9A-Fa-f]{1,2}");
    QRegularExpressionMatchIterator i = hexRegex.globalMatch(codeText);

    QVector<uint8_t> bytes;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        bytes.append(match.captured(0).toUShort(nullptr, 16));
    }

    if (bytes.isEmpty()) {
        QMessageBox::warning(m_parentWindow, "Error", "No valid data in 0xFF format found.");
        return;
    }

    int byteIdx = 0;
    if (isU8g2) {
        for (int page = 0; page < m_projectHeight / 8; ++page) {
            for (int x = 0; x < m_projectWidth; ++x) {
                if (byteIdx >= bytes.size()) break;
                uint8_t b = bytes[byteIdx++];
                for (int bit = 0; bit < 8; ++bit) {
                    if (b & (1 << bit)) img.setPixelColor(x, page * 8 + bit, Qt::white);
                }
            }
        }
    } else {
        for (int y = 0; y < m_projectHeight; ++y) {
            for (int x = 0; x < m_projectWidth; x += 8) {
                if (byteIdx >= bytes.size()) break;
                uint8_t b = bytes[byteIdx++];
                for (int bit = 0; bit < 8; ++bit) {
                    if (b & (1 << (7 - bit))) img.setPixelColor(x + bit, y, Qt::white);
                }
            }
        }
    }

    if (m_model) {
        m_model->setClipboardImage(img);
        emit requestPasteToLayer();
    }
}
