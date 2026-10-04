#include "filecontroller.h"
#include "pixelcanvas.h"
#include "projectmodel.h"
#include "createprojectdialog.h"
#include "settingsmanager.h"

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
        saveProject();

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
    QString lastDir = SettingsManager::getLastDirectory();

    QString defaultFileName = m_currentProjectName.isEmpty() ? "Untitled.pod2d" : m_currentProjectName;
    if (!defaultFileName.endsWith(".pod2d") && !defaultFileName.endsWith(".png")) {
        defaultFileName += ".pod2d";
    }
    QString fullPath = lastDir + "/" + defaultFileName;

    QString filePath = QFileDialog::getSaveFileName(
        m_parentWindow, tr("Save project as..."), fullPath,
        tr("POD2d Project (*.pod2d);;PNG Image (*.png);;All Files (*)")
        );

    if (filePath.isEmpty()) return;

    SettingsManager::setLastDirectory(QFileInfo(filePath).absolutePath());

    m_currentFilePath = filePath;
    m_currentProjectName = QFileInfo(filePath).baseName();

    saveProject();
}

void FileController::saveProject() {
    if (m_currentFilePath.isEmpty()) {
        saveProjectAs();
        return;
    }

    if (m_currentFilePath.endsWith(".png", Qt::CaseInsensitive)) {
        QImage img = m_model->getFlattenedImage();

        if (!m_model->getIsRGB()) {
            QColor monoColor = m_parentWindow->findChild<PixelCanvas*>("canvasWidget")->getMonoDisplayColor();
            if (monoColor != Qt::white) {
                img = img.convertToFormat(QImage::Format_ARGB32);
                const QRgb whiteRgb = qRgba(255, 255, 255, 255);
                const QRgb targetRgb = monoColor.rgba();
                for (int y = 0; y < img.height(); ++y) {
                    QRgb *line = reinterpret_cast<QRgb*>(img.scanLine(y));
                    for (int x = 0; x < img.width(); ++x) {
                        if (line[x] == whiteRgb) {
                            line[x] = targetRgb;
                        }
                    }
                }
            }
        }

        if (!img.save(m_currentFilePath, "PNG")) {
            QMessageBox::critical(m_parentWindow, tr("Error"), tr("Failed to save PNG file."));
            return;
        }
    } else {
        QByteArray projectData = m_model->saveProjectData();
        QFile file(m_currentFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(m_parentWindow, tr("Error"), tr("Failed to save the file. Check access permissions."));
            return;
        }
        file.write(projectData);
        file.close();
    }

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
                                                      "Insert your array (e.g., 0xFF, 0x9914...):", "", &ok);
    if (!ok || codeText.isEmpty()) return;

    QSettings settings("POD2d", "EditorSettings");
    bool isU8g2 = (settings.value("export/byteFormat", 0).toInt() == 1);

    QImage img(m_projectWidth, m_projectHeight, QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QRegularExpression hexRegex("0[xX][0-9A-Fa-f]+");
    QRegularExpressionMatchIterator i = hexRegex.globalMatch(codeText);

    QVector<uint16_t> parsedData;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        parsedData.append(match.captured(0).toUShort(nullptr, 16));
    }

    if (parsedData.isEmpty()) {
        QMessageBox::warning(m_parentWindow, "Error", "No valid data found.");
        return;
    }

    int byteIdx = 0;

    if (m_model && m_model->getIsRGB()) {
        for (int y = 0; y < m_projectHeight; ++y) {
            for (int x = 0; x < m_projectWidth; ++x) {
                if (byteIdx >= parsedData.size()) break;

                uint16_t color565 = parsedData[byteIdx++];

                int r5 = (color565 >> 11) & 0x1F;
                int g6 = (color565 >> 5) & 0x3F;
                int b5 = color565 & 0x1F;

                int r8 = (r5 * 255) / 31;
                int g8 = (g6 * 255) / 63;
                int b8 = (b5 * 255) / 31;

                img.setPixelColor(x, y, QColor(r8, g8, b8));
            }
        }
    }
    else {
        if (isU8g2) {
            for (int page = 0; page < m_projectHeight / 8; ++page) {
                for (int x = 0; x < m_projectWidth; ++x) {
                    if (byteIdx >= parsedData.size()) break;
                    uint8_t b = static_cast<uint8_t>(parsedData[byteIdx++]);
                    for (int bit = 0; bit < 8; ++bit) {
                        if (b & (1 << bit)) img.setPixelColor(x, page * 8 + bit, Qt::white);
                    }
                }
            }
        } else {
            for (int y = 0; y < m_projectHeight; ++y) {
                for (int x = 0; x < m_projectWidth; x += 8) {
                    if (byteIdx >= parsedData.size()) break;
                    uint8_t b = static_cast<uint8_t>(parsedData[byteIdx++]);
                    for (int bit = 0; bit < 8; ++bit) {
                        if (x + bit < m_projectWidth) {
                            if (b & (1 << (7 - bit))) img.setPixelColor(x + bit, y, Qt::white);
                        }
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

void FileController::renameProject() {
    if (m_currentFilePath.isEmpty()) return;

    bool ok;
    QString newName = QInputDialog::getText(m_parentWindow, tr("Rename Project"),
                                            tr("New name (without extension):"),
                                            QLineEdit::Normal,
                                            m_currentProjectName, &ok);

    if (ok && !newName.isEmpty() && newName != m_currentProjectName) {
        QFileInfo fileInfo(m_currentFilePath);
        QString newPath = fileInfo.absolutePath() + "/" + newName + "." + fileInfo.completeSuffix();

        if (QFile::exists(newPath)) {
            QMessageBox::warning(m_parentWindow, tr("Error"), tr("A file with this name already exists!"));
            return;
        }

        if (QFile::rename(m_currentFilePath, newPath)) {
            m_currentFilePath = newPath;
            m_currentProjectName = newName;

            emit recentProjectAdded(newPath);
            emit stateChanged();
        } else {
            QMessageBox::critical(m_parentWindow, tr("Error"), tr("Failed to rename the file. Check if it is open in another program."));
        }
    }
}
