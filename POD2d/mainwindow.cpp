#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "pixelcanvas.h"
#include "codegenerator.h"
#include "projectmodel.h"
#include "createprojectdialog.h"
#include "exportdialog.h"
#include "settingsdialog.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QColorDialog>
#include <QFileInfo>
#include <QDir>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QClipboard>
#include <QGuiApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("POD2d");

    initModels();
    setupWidgets();
    loadIcons();
    setupConnections();

    rebuildLayersList();
}

MainWindow::~MainWindow() {
    delete ui;
}

// ==========================================
// 1. Initialization and configuration
// ==========================================

void MainWindow::initModels() {
    projectModel = new ProjectModel(this);
    ui->canvasWidget->setModel(projectModel);

    this->setWindowTitle("POD2d");
    this->showMaximized();
    setEditorUIEnabled(false);
}

void MainWindow::setupWidgets() {
    setupFramesListWidget();
    setupLayersListWidget();
}

void MainWindow::setupFramesListWidget() {
    QListWidget* framesList = ui->framesListWidget;

    framesList->setViewMode(QListView::IconMode);
    framesList->setFlow(QListView::LeftToRight);
    framesList->setIconSize(QSize(128, 64));
    framesList->setGridSize(QSize(140, 90));
    framesList->setSpacing(5);
    framesList->setFixedHeight(100);
    framesList->setMovement(QListView::Static);

    QListWidgetItem *firstFrameItem = new QListWidgetItem("1");
    firstFrameItem->setIcon(QIcon(QPixmap::fromImage(projectModel->getFlattenedImage())));

    framesList->addItem(firstFrameItem);
    framesList->setCurrentRow(0);
}

void MainWindow::setupLayersListWidget() {
    QListWidget* layersList = ui->layersListWidget;

    layersList->setViewMode(QListView::ListMode);
    layersList->setIconSize(QSize(64, 32));
    layersList->setSpacing(3);
}

void MainWindow::loadIcons() {
    QString basePath = QFileInfo(__FILE__).dir().absolutePath();

    ui->btn_Undo->setIcon(QIcon(basePath + "/image/undo.png"));
    ui->btn_Redo->setIcon(QIcon(basePath + "/image/redo.png"));
    ui->btn_Paste->setIcon(QIcon(basePath + "/image/paste.png"));
    ui->btn_Copy->setIcon(QIcon(basePath + "/image/copy.png"));
    ui->btn_Cut->setIcon(QIcon(basePath + "/image/cut.png"));

    ui->btn_Pen->setIcon(QIcon(basePath + "/image/pen.png"));
    ui->btn_Dithering->setIcon(QIcon(basePath + "/image/dithering.png"));
    ui->btn_Fill->setIcon(QIcon(basePath + "/image/pain.png"));
    ui->btn_Text->setIcon(QIcon(basePath + "/image/text.png"));
    ui->btn_Line->setIcon(QIcon(basePath + "/image/line.png"));
    ui->btn_BrokenLine->setIcon(QIcon(basePath + "/image/brokenLine.png"));
    ui->btn_Circle->setIcon(QIcon(basePath + "/image/circle.png"));
    ui->btn_Rectangle->setIcon(QIcon(basePath + "/image/rectangle.png"));
    ui->btn_Pan->setIcon(QIcon(basePath + "/image/pan.png"));
    ui->btn_Rotate->setIcon(QIcon(basePath + "/image/rotate.png"));
    ui->btm_Play->setIcon(QIcon(basePath + "/image/play.png"));
}

// ==========================================
// 2. Connecting signals and slots
// ==========================================

void MainWindow::setupConnections() {
    connectModelToLists();
    connectMenuButtons();
    connectEditorControls();
    connectDrawingTools();
    connectActions();
    connectAutoSaveTimer();
}

void MainWindow::connectModelToLists() {
    connectFramesList();
    connectLayersList();
    connectMiniCanvas();

    connect(projectModel, &ProjectModel::imageChanged, this, [this]() {
        ui->canvasWidget->update();
    });
    connect(projectModel, &ProjectModel::projectModified, this, &MainWindow::markProjectAsModified);
}

void MainWindow::connectFramesList() {
    QListWidget* framesList = ui->framesListWidget;

    connect(projectModel, &ProjectModel::framesListChanged, this, &MainWindow::rebuildFramesList);

    connect(projectModel, &ProjectModel::frameChanged, this, [this, framesList](int index) {
        framesList->blockSignals(true);
        framesList->setCurrentRow(index);
        framesList->blockSignals(false);
        rebuildLayersList();
    });

    connect(framesList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) {
            projectModel->setCurrentFrame(row);
            ui->canvasWidget->update();
        }
    });

    connect(projectModel, &ProjectModel::forceUIFrameSelection, this, [this](int index) {
        ui->framesListWidget->blockSignals(true);
        ui->framesListWidget->setCurrentRow(index);
        ui->framesListWidget->blockSignals(false);
    });

    connect(projectModel, &ProjectModel::imageChanged, this, [this](const QImage &flatImg) {
        if (QListWidgetItem *currentItem = ui->framesListWidget->currentItem()) {
            if (QWidget *cellWidget = ui->framesListWidget->itemWidget(currentItem)) {
                if (QLabel *imgLabel = cellWidget->findChild<QLabel*>("frameImage")) {
                    QPixmap newPix = QPixmap::fromImage(flatImg).scaled(128, 64, Qt::KeepAspectRatio, Qt::FastTransformation);
                    imgLabel->setPixmap(newPix);
                }
            }
        }
    });
}

void MainWindow::connectLayersList() {
    QListWidget* layersList = ui->layersListWidget;

    connect(projectModel, &ProjectModel::layersListChanged, this, &MainWindow::rebuildLayersList);

    connect(projectModel, &ProjectModel::activeLayerChanged, this, [layersList](int index) {
        layersList->blockSignals(true);
        if (index >= 0 && index < layersList->count()) {
            layersList->setCurrentRow(index);
        }
        layersList->blockSignals(false);
    });

    connect(projectModel, &ProjectModel::layerThumbnailUpdated, this, [this, layersList](int index) {
        if (index >= 0 && index < layersList->count()) {
            QImage thumb = projectModel->getLayerThumbnail(index);
            layersList->item(index)->setIcon(QIcon(QPixmap::fromImage(thumb)));
        }
    });

    connect(layersList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) {
            projectModel->setCurrentLayer(row);
            ui->canvasWidget->update();
        }
    });

    connect(projectModel, &ProjectModel::forceUILayerSelection, this, [this](int index) {
        ui->layersListWidget->blockSignals(true);
        ui->layersListWidget->setCurrentRow(index);
        ui->layersListWidget->blockSignals(false);
    });

    connect(projectModel, &ProjectModel::imageChanged, this, [this](const QImage &flatImg) {
        if (QListWidgetItem *currentItem = ui->framesListWidget->currentItem()) {

            QImage scaledImg = flatImg.scaled(128, 64, Qt::KeepAspectRatio, Qt::FastTransformation);

            currentItem->setData(Qt::UserRole, QPixmap::fromImage(scaledImg));
        }
    });
}

void MainWindow::connectMiniCanvas() {
    QListWidget* framesList = ui->framesListWidget;
    QLabel* miniCanvas = ui->miniCanvasWidget;

    QImage initialImg = projectModel->getFlattenedImage();
    QPixmap initialPixmap = QPixmap::fromImage(initialImg).scaled(
        miniCanvas->size(),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
        );
    miniCanvas->setPixmap(initialPixmap);

    connect(projectModel, &ProjectModel::imageChanged, this, [framesList, miniCanvas](const QImage &img) {
        if (QListWidgetItem *currentItem = framesList->currentItem()) {
            currentItem->setIcon(QIcon(QPixmap::fromImage(img)));
        }

        QPixmap pixmap = QPixmap::fromImage(img).scaled(
            miniCanvas->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
            );

        miniCanvas->setPixmap(pixmap);
    });
}

void MainWindow::connectMenuButtons() {
    connect(ui->btn_CreateProject, &QPushButton::clicked, this, &MainWindow::createProject);
    connect(ui->btn_OpenProject,   &QPushButton::clicked, this, &MainWindow::openProject);
    connect(ui->btn_ProjectsTab,   &QPushButton::clicked, this, &MainWindow::buttonProjects);
    connect(ui->btn_ExamplesTab,   &QPushButton::clicked, this, &MainWindow::buttonExamples);
}

void MainWindow::connectEditorControls() {

    connect(ui->btn_Undo,  &QPushButton::clicked, this, &MainWindow::undo);
    connect(ui->btn_Redo,  &QPushButton::clicked, this, &MainWindow::redo);
    connect(ui->btn_Clear, &QPushButton::clicked, this, &MainWindow::clear);

    connect(ui->btn_AddLayer,    &QPushButton::clicked, this, &MainWindow::addLayer);
    connect(ui->btn_AddFrame,    &QPushButton::clicked, projectModel, &ProjectModel::addFrame);
    connect(ui->btn_DeleteFrame, &QPushButton::clicked, projectModel, &ProjectModel::deleteCurrentFrame);
    connect(ui->btn_DeleteLayer, &QPushButton::clicked, projectModel, &ProjectModel::deleteCurrentLayer);

    connect(ui->btn_Paste,  &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::pasteToLayer);
    connect(ui->btn_Copy,   &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::copyLayer);
    connect(ui->btn_Cut,    &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::cutLayer);
    connect(ui->btn_Rotate, &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::rotateFloatingImage);

    connect(ui->btn_ExportCode, &QPushButton::clicked, this, &MainWindow::openExportMenu);

    connectPlayerControls();
}

void MainWindow::connectPlayerControls() {
    QPushButton* playBtn = ui->btm_Play;

    connect(playBtn, &QPushButton::clicked, projectModel, &ProjectModel::togglePlay);

    connect(projectModel, &ProjectModel::isPlayingChanged, this, [playBtn](bool playing) {
        QString basePath = QFileInfo(__FILE__).dir().absolutePath();
        QString iconPath = playing ? basePath + "/image/stop.png" : basePath + "/image/play.png";
        playBtn->setIcon(QIcon(iconPath));
    });
}

void MainWindow::connectAutoSaveTimer() {
    autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, &QTimer::timeout, this, [this]() {
        if (isProjectModified && !currentFilePath.isEmpty()) {
            saveProject();
        }
    });

    applySettings();
}


void MainWindow::connectDrawingTools() {
    connect(ui->btn_Pen,        &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Brush); });
    connect(ui->btn_Dithering,  &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Dithering); });
    connect(ui->btn_Line,       &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Line); });
    connect(ui->btn_Rectangle,  &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Rectangle); });
    connect(ui->btn_Circle,     &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Circle); });
    connect(ui->btn_Fill,       &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Fill); });
    connect(ui->btn_BrokenLine, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::BrokenLine); });
    connect(ui->btn_Text,       &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Text); });
    connect(ui->btn_Pan,        &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Pan); });
    connect(ui->btn_Select,     &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Select); });

    connect(ui->spin_BrushSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::on_spin_brushSize_valueChanged);
}

void MainWindow::connectActions() {
    connectFileActions();
    connectEditActions();
    connectViewActions();
    connectPreferencesActions();
    connectHelpActions();
}

void MainWindow::connectFileActions() {
    ui->act_NewFile->setShortcut(QKeySequence::New);
    ui->act_Save->setShortcut(QKeySequence::Save);
    ui->act_SaveAs->setShortcut(QKeySequence("Ctrl+Shift+S"));
    ui->act_ExportCode->setShortcut(QKeySequence("Ctrl+E"));
    ui->act_OpenFile->setShortcut(QKeySequence::Open);
    ui->act_Close->setShortcut(QKeySequence("Ctrl+W"));
    ui->act_Exit->setShortcut(QKeySequence("Alt+F4"));

    connect(ui->act_NewFile, &QAction::triggered, this, &MainWindow::createProject);
    connect(ui->act_Save, &QAction::triggered, this, &MainWindow::saveProject);
    connect(ui->act_SaveAs, &QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(ui->act_ExportCode, &QAction::triggered, this, &MainWindow::openExportMenu);
    connect(ui->act_OpenFile, &QAction::triggered, this, &MainWindow::openProject);
    connect(ui->act_Close, &QAction::triggered, this, &MainWindow::closeProject);
    connect(ui->act_Exit, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::connectEditActions() {
    ui->act_Undo->setShortcut(QKeySequence::Undo);
    ui->act_Redo->setShortcut(QKeySequence::Redo);

    ui->act_Select->setShortcut(QKeySequence::SelectAll);
    ui->act_Cut->setShortcut(QKeySequence::Cut);
    ui->act_Copy->setShortcut(QKeySequence::Copy);
    ui->act_Paste->setShortcut(QKeySequence::Paste);

    ui->act_Pen->setShortcut(QKeySequence("P"));
    ui->act_Line->setShortcut(QKeySequence("L"));
    ui->act_Text->setShortcut(QKeySequence("T"));
    ui->act_Fill->setShortcut(QKeySequence("F"));

    connect(ui->act_Undo, &QAction::triggered, projectModel, &ProjectModel::undo);
    connect(ui->act_Redo, &QAction::triggered, projectModel, &ProjectModel::redo);

    connect(ui->act_Select, &QAction::triggered, this, &MainWindow::selectAll);
    connect(ui->act_Cut, &QAction::triggered, ui->canvasWidget, &PixelCanvas::cutLayer);
    connect(ui->act_Copy, &QAction::triggered, ui->canvasWidget, &PixelCanvas::copyLayer);
    connect(ui->act_Paste, &QAction::triggered, ui->canvasWidget, &PixelCanvas::pasteToLayer);

    connect(ui->act_Pen, &QAction::triggered, this, [this](){ ui->canvasWidget->setTool(DrawTool::Pen); });
    connect(ui->act_Line, &QAction::triggered, this, [this](){ ui->canvasWidget->setTool(DrawTool::Line); });
    connect(ui->act_Text, &QAction::triggered, this, [this](){ ui->canvasWidget->setTool(DrawTool::Text); });
    connect(ui->act_Fill, &QAction::triggered, this, [this](){ ui->canvasWidget->setTool(DrawTool::Fill); });
}

void MainWindow::connectViewActions() {
    connect(ui->act_ViewMiniMap, &QAction::triggered, this, [this]() {
        bool isVisible = ui->miniCanvasWidget->isVisible();

        setMiniMapVisible(!isVisible);
    });

    connect(ui->act_ViewFrames, &QAction::triggered, this, [this]() {
        bool isVisible = ui->framesListWidget->isVisible();
        setFrameListVisible(!isVisible);
    });

    connect(ui->act_ViewLayers, &QAction::triggered, this, [this]() {
        bool isVisible = ui->layersListWidget->isVisible();
        setLayerListVisible(!isVisible);
    });

    connect(ui->act_ViewTools, &QAction::triggered, this, [this]() {
        bool isVisible = ui->frm_Tools->isVisible();
        setToolsVisible(!isVisible);
    });
}

void MainWindow::connectPreferencesActions(){

    connect(ui->act_Settings, &QAction::triggered, this, [this]() {
        openSettings(0);
    });

    connect(ui->act_KeyBindings, &QAction::triggered, this, [this]() {
        openSettings(1);
    });

    connect(ui->act_MouseBindings, &QAction::triggered, this, [this]() {
        openSettings(2);
    });

    connect(ui->act_SetColor, &QAction::triggered, this, &MainWindow::chooseAndSetColor);
}

void MainWindow::connectHelpActions() {
    connect(ui->act_HelpDocs, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/ilertwo/POD2d/wiki"));
    });

    connect(ui->act_HelpBug, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/ilertwo/POD2d/issues/new"));
    });

    connect(ui->act_HelpIdea, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/ilertwo/POD2d/issues"));
    });

    connect(ui->act_HelpUpdates, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, "Check for Updates",
                                 "You are using the latest version of POD2d.");// TODO:********************************************************
    });

    connect(ui->act_HelpAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About POD2d",
                           "<b>POD2d</b> - Pixel OLED Designer<br><br>"
                           "Версія: 1.0.0<br>"//TODO:******************************************************************************************
                           "Автор: Ilertwo<br><br>"
                           "Created using C++ and Qt.");
    });

    connect(ui->act_HelpUkraine, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(QUrl("https://u24.gov.ua/"));
    });
}

// ==========================================
// 3. Logic methods and handlers
// ==========================================

// Group A: File & Project Management
// ====================================
void MainWindow::createProject() {
    CreateProjectDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {

        currentFilePath.clear();
        currentFilePath = dialog.getFullFilePath();

        currentProjectName = dialog.getProjectName();

        projectWidth = dialog.getWidth();
        projectHeight = dialog.getHeight();

        if (currentProjectName.isEmpty()) {
            currentProjectName = "Untitled";
        }

        this->setWindowTitle("POD2d - " + currentProjectName);

        if (projectModel) {
            projectModel->initDefaultProject(projectWidth, projectHeight);
            projectModel->setCanvasSize(projectWidth, projectHeight);
            ui->canvasWidget->setCanvasSize(projectWidth, projectHeight);
            updateUIProportions(projectWidth, projectHeight);
            projectModel->notifyImageChanged();
        }

        ui->canvasWidget->resetToolState();
        ui->canvasWidget->setTool(DrawTool::Brush);

        rebuildFramesList();
        rebuildLayersList();

        setEditorUIEnabled(true);

        ui->stackedWidget->setCurrentIndex(1);

        QTimer::singleShot(50, this, [this]() {
            ui->canvasWidget->fitToScreen();
            ui->canvasWidget->update();
        });

        isProjectModified = false;
    }
}

void MainWindow::openProject() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "Pod2D Project (*.pod2d)"
        );

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open the file!");
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    if (projectModel->loadProjectData(data)) {
        currentFilePath = path;
        QFileInfo fileInfo(path);
        currentProjectName = fileInfo.baseName();
        this->setWindowTitle("POD2d - " + currentProjectName);

        QImage loadedImg = projectModel->getActiveLayerImage();
        projectWidth = loadedImg.width();
        projectHeight = loadedImg.height();

        projectModel->setCanvasSize(projectWidth, projectHeight);
        ui->canvasWidget->setCanvasSize(projectWidth, projectHeight);
        updateUIProportions(projectWidth, projectHeight);

        ui->canvasWidget->resetToolState();
        ui->canvasWidget->setTool(DrawTool::Brush);
        rebuildLayersList();

        rebuildFramesList();

        setEditorUIEnabled(true);

        ui->stackedWidget->setCurrentIndex(1);

        QTimer::singleShot(50, this, [this]() {
            ui->canvasWidget->fitToScreen();
            ui->canvasWidget->update();
        });

        isProjectModified = false;
    } else {
        QMessageBox::warning(this, "Error", "The project file is corrupted or has an invalid format!");
    }
}

void MainWindow::saveProjectAs() {
    QString defaultFileName = currentProjectName;
    if (defaultFileName.isEmpty()) {
        defaultFileName = "Untitled";
    }
    defaultFileName += ".pod2d";

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save project as...",
        defaultFileName,
        "POD2d Project (*.pod2d);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    currentFilePath = filePath;
    QFileInfo fileInfo(filePath);
    currentProjectName = fileInfo.baseName();

    saveProject();
}

void MainWindow::saveProject() {
    if (currentFilePath.isEmpty()) {
        saveProjectAs();
        return;
    }

    QByteArray projectData = projectModel->saveProjectData();

    QFile file(currentFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Failed to save the file. Check access permissions.");
        return;
    }

    file.write(projectData);
    file.close();

    QFileInfo fileInfo(currentFilePath);
    this->setWindowTitle("POD2d - " + fileInfo.fileName());
    isProjectModified = false;
}

void MainWindow::closeProject() {
    if (!maybeSave()) {
        return;
    }

    ui->stackedWidget->setCurrentIndex(0);

    setEditorUIEnabled(false);

    currentFilePath.clear();
    currentProjectName.clear();
    isProjectModified = false;
    this->setWindowTitle("POD2d");

    if (projectModel) {// DELETE*********************************************************************************
        rebuildFramesList();
        rebuildLayersList();

        ui->canvasWidget->resetToolState();
        ui->canvasWidget->update();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::maybeSave() {
    if (ui->stackedWidget->currentIndex() == 0 || !isProjectModified) {
        return true;
    }

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, "POD2d",
                               "You have unsaved changes. Do you want to save them before exiting?",
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        saveProject();
        return true;
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }

    return true;
}

void MainWindow::markProjectAsModified() {
    if (!isProjectModified) {
        isProjectModified = true;
        QString titleName = currentProjectName.isEmpty() ? "Untitled" : currentProjectName;
        this->setWindowTitle("POD2d - " + titleName + "*");
    }
}

void MainWindow::openSettings(int tabIndex) {
    SettingsDialog dialog(this);

    dialog.setActiveTab(tabIndex);

    if (dialog.exec() == QDialog::Accepted) {
        applySettings();
    }
}

void MainWindow::openKeyBindings(int tabIndex) {
    SettingsDialog dialog(this);

    dialog.setActiveTab(tabIndex);

    if (dialog.exec() == QDialog::Accepted) {
    }
}

void MainWindow::applySettings() {
    QSettings settings("POD2d", "EditorSettings");
    bool autoSaveEnabled = settings.value("editor/autoSave", false).toBool();
    int intervalMinutes = settings.value("editor/autoSaveInterval", 5).toInt();

    if (autoSaveEnabled) {
        autoSaveTimer->start(intervalMinutes * 60 * 1000);
    } else {
        autoSaveTimer->stop();
    }

    projectModel->loadSettings();
}

// Group B: UI & State Updates
// ====================================
void MainWindow::rebuildLayersList() {
    QListWidget* layersList = ui->layersListWidget;

    const QSignalBlocker blocker(layersList);

    layersList->clear();

    const int count = projectModel->getLayerCount();

    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = new QListWidgetItem(QString::number(i));

        QImage thumb = projectModel->getLayerThumbnail(i);
        item->setIcon(QIcon(QPixmap::fromImage(thumb)));

        layersList->addItem(item);
    }

    layersList->setCurrentRow(projectModel->getCurrentLayerIndex());
}

void MainWindow::rebuildFramesList() {
    QListWidget* framesList = ui->framesListWidget;
    const QSignalBlocker blocker(framesList);
    framesList->clear();

    const int frameCount = projectModel->getFrameCount();

    framesList->setSpacing(5);
    framesList->setStyleSheet(
        "QListWidget { outline: 0; background: transparent; }"
        "QListWidget::item:selected { border: 2px solid #666666; border-radius: 6px; }"
        );

    for (int i = 0; i < frameCount; ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(140, 100));
        framesList->addItem(item);

        QFrame *frameWidget = new QFrame();
        frameWidget->setStyleSheet(
            "QFrame {"
            "   background-color: #333333;"
            "   border-radius: 6px;"
            "}"
            );

        QVBoxLayout *layout = new QVBoxLayout(frameWidget);
        layout->setContentsMargins(5, 5, 5, 2);
        layout->setSpacing(2);

        QLabel *imageLabel = new QLabel();

        imageLabel->setObjectName("frameImage");

        QImage thumb = projectModel->getFrameThumbnail(i);
        QPixmap pixmap = QPixmap::fromImage(thumb).scaled(128, 64, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pixmap);
        imageLabel->setAlignment(Qt::AlignCenter);

        QLabel *textLabel = new QLabel(QString::number(i + 1));
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setStyleSheet("color: white; font-size: 10px; border: none; background: transparent;");

        layout->addWidget(imageLabel);
        layout->addWidget(textLabel);

        framesList->setItemWidget(item, frameWidget);
    }

    framesList->setCurrentRow(projectModel->getCurrentFrameIndex());
}

void MainWindow::setEditorUIEnabled(bool enabled) {
    ui->act_Save->setEnabled(enabled);
    ui->act_SaveAs->setEnabled(enabled);
    ui->act_Undo->setEnabled(enabled);
    ui->act_Redo->setEnabled(enabled);
    ui->act_Select->setEnabled(enabled);
    ui->act_Cut->setEnabled(enabled);
    ui->act_Copy->setEnabled(enabled);
    ui->act_Paste->setEnabled(enabled);

    ui->act_Pen->setEnabled(enabled);
    ui->act_Line->setEnabled(enabled);
    ui->act_Text->setEnabled(enabled);
    ui->act_Fill->setEnabled(enabled);

    ui->act_ViewMiniMap->setEnabled(enabled);
    ui->act_ViewFrames->setEnabled(enabled);
    ui->act_ViewLayers->setEnabled(enabled);
    ui->act_ViewTools->setEnabled(enabled);
}

void MainWindow::setMiniMapVisible(bool visible) {
    ui->miniCanvasFrame->setVisible(visible);

    QString text = visible ? "Hide Minimap" : "Show Minimap";
    ui->act_ViewMiniMap->setText(text);
}

void MainWindow::setFrameListVisible(bool visible) {
    ui->framesListWidget->setVisible(visible);
    ui->frm_AddDeleteFrame->setVisible(visible);

    QString text = visible ? "Hide Frames" : "Show Frames";
    ui->act_ViewFrames->setText(text);
}

void MainWindow::setLayerListVisible(bool visible) {
    ui->layersListWidget->setVisible(visible);
    ui->layersLabel->setVisible(visible);
    ui->frm_AddDeleteLayer->setVisible(visible);
    ui->layersPanelWidget->setVisible(visible);

    QString text = visible ? "Hide Layers" : "Show Layers";
    ui->act_ViewLayers->setText(text);
}

void MainWindow::setToolsVisible(bool visible) {
    ui->frm_Tools->setVisible(visible);
    ui->miniCanvasFrame->setVisible(visible);
    ui->toolsLabel->setVisible(visible);

    QString text = visible ? "Hide Tools" : "Show Tools";
    ui->act_ViewTools->setText(text);
    ui->act_ViewMiniMap->setEnabled(visible);
}

void MainWindow::updateUIProportions(int projWidth, int projHeight) {
    if (projHeight == 0) return;

    int baseIconHeight = 64;
    int proportionalIconWidth = (projWidth * baseIconHeight) / projHeight;
    QSize newIconSize(proportionalIconWidth, baseIconHeight);

    //ui->framesListWidget->setIconSize(newIconSize);
    ui->layersListWidget->setIconSize(newIconSize);

    rebuildFramesList();
    rebuildLayersList();

    QImage currentImg = projectModel->getFlattenedImage();
    if (!currentImg.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(currentImg).scaled(
            ui->miniCanvasWidget->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
            );
        ui->miniCanvasWidget->setPixmap(pixmap);
    }
}

// Group C: Editor Controls & Tools
// ====================================
void MainWindow::undo() {
    projectModel->undo();
    ui->canvasWidget->update();
}

void MainWindow::redo() {
    projectModel->redo();
    ui->canvasWidget->update();
}

void MainWindow::clear() {
    projectModel->clearCanvas();
    ui->canvasWidget->resetToolState();
}

void MainWindow::selectAll() {
    QImage currentLayerImage = projectModel->getCurrentLayerImage();

    if (currentLayerImage.isNull()) {
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setImage(currentLayerImage);
}

void MainWindow::setScale(int newScale) {
    if (newScale < 1) return;

    ui->canvasWidget->update();
}

void MainWindow::on_spin_brushSize_valueChanged(int value) { ui->canvasWidget->setBrushSize(value); }

void MainWindow::chooseAndSetColor() {
    const QColor selectedColor = QColorDialog::getColor(Qt::white, this, "Choose a color");

    if (selectedColor.isValid()) {
        // ui->canvasWidget->setCurrentColor(selectedColor);
    }
}

// Group D: Layer Management (Delegations to ProjectModel)
// ====================================
void MainWindow::addLayer() {
    projectModel->addLayer();
}

void MainWindow::deleteCurrentLayer() { projectModel->deleteCurrentLayer(); }

// Group E: Navigation & Dialogs
// ====================================
void MainWindow::buttonCreate() {
    ui->stackedWidget->setCurrentIndex(1);

}

void MainWindow::buttonCancel() {
}

void MainWindow::buttonProjects() {
    // TODO
}

void MainWindow::buttonExamples() {
    // TODO
}

void MainWindow::recentProject() {
    // TODO
}

void MainWindow::openExportMenu() {
    ExportDialog dialog(projectModel, this);

    dialog.exec();
}
