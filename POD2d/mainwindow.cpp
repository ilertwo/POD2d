#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "pixelcanvas.h"
#include "codegenerator.h"
#include "projectmodel.h"
#include "createprojectdialog.h"
#include "exportdialog.h"
#include "settingsdialog.h"
#include "palettedialog.h"

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
#include <QTextStream>
#include <QRegularExpression>
#include <QFile>
#include <QApplication>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("POD2d");

    initModels();
    setupTheme();
    setupWidgets();
    loadIcons();
    setupConnections();
    setupPalette();

    rebuildLayersList();
    updateRecentProjectsUI();

    ui->stackedWidget->setCurrentIndex(0);
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

void MainWindow::setupTheme() {
    QSettings settings("POD2d", "EditorSettings");
    QString currentTheme = settings.value("ui/theme", "dark").toString(); // dark - тема за замовчуванням
    applyTheme(currentTheme);
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

void MainWindow::setupPalette() {
    customPalette = {
        QColor(0, 0, 0), QColor(255, 255, 255), QColor(255, 0, 0),
        QColor(0, 255, 0), QColor(0, 0, 255), QColor(255, 255, 0)
    };
    rebuildPaletteGrid();

    ui->widget_Palette->installEventFilter(this);
}

void MainWindow::loadIcons() {
    QString basePath = QFileInfo(__FILE__).dir().absolutePath();
    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

    QFont pixelFont("Courier New", 14, QFont::Bold);

    auto setBtnIcon = [&](QPushButton* btn, const QString& text, const QString& iconName) {
        if (theme == "1bit") {
            btn->setIcon(QIcon());
            btn->setText(text);
            btn->setFont(pixelFont);
        } else {
            btn->setText("");
            QString fullPath = basePath + "/image/" + theme + "/" + iconName;
            btn->setIcon(QIcon(fullPath));
        }
    };

    setBtnIcon(ui->btn_Save, "S", "save.png");
    setBtnIcon(ui->btn_Undo, "<", "undo.png");
    setBtnIcon(ui->btn_Redo, ">", "redo.png");

    setBtnIcon(ui->btn_RectangleSelection, "[:]", "select.png");
    setBtnIcon(ui->btn_LassoSelection, "@", "lasso.png");
    setBtnIcon(ui->btn_ShapeSelection, "!", "shape.png");
    setBtnIcon(ui->btn_Lighten, "*", "lighten.png");

    setBtnIcon(ui->btn_Pen, "/", "pen.png");
    setBtnIcon(ui->btn_Eraser, "=", "eraser.png");
    setBtnIcon(ui->btn_Pipette, "j", "pipette.png");
    setBtnIcon(ui->btn_Dithering, "#", "dithering.png");
    setBtnIcon(ui->btn_Fill, "U", "pain.png");
    setBtnIcon(ui->btn_Text, "A", "text.png");
    setBtnIcon(ui->btn_Line, "\\", "line.png");
    setBtnIcon(ui->btn_BrokenLine, "N", "brokenLine.png");
    setBtnIcon(ui->btn_Circle, "O", "circle.png");
    setBtnIcon(ui->btn_Rectangle, "[]", "rectangle.png");

    setBtnIcon(ui->btn_Pan, "W", "pan.png");
    setBtnIcon(ui->btn_Rotate, "G", "rotate.png");
    setBtnIcon(ui->btn_VerticalMiror, "|", "mirror.png");

    setBtnIcon(ui->btn_HideMiniMap, "m", "hide_minimap.png");
    setBtnIcon(ui->btn_HideFrames, "f", "hide_frames.png");
    setBtnIcon(ui->btn_HideLayers, "l", "hide_layers.png");
    setBtnIcon(ui->btn_EditMode, "E", "edit_mode.png");

    bool isPlaying = projectModel && projectModel->isPlaying();
    if (isPlaying) {
        setBtnIcon(ui->btn_Play, "X", "stop.png");
    } else {
        setBtnIcon(ui->btn_Play, ">", "play.png");
    }
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
    connectRecentProjects();

    setupShortcuts();
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
            QListWidgetItem *item = layersList->item(index);

            if (QWidget *cellWidget = layersList->itemWidget(item)) {
                if (QLabel *imgLabel = cellWidget->findChild<QLabel*>("layerImage")) {
                    QPixmap newPix = QPixmap::fromImage(thumb).scaled(100, 32, Qt::KeepAspectRatio, Qt::FastTransformation);
                    imgLabel->setPixmap(newPix);
                }
            }
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
    //connect(ui->btn_Clear, &QPushButton::clicked, this, &MainWindow::clear);

    connect(ui->btn_AddLayer,    &QPushButton::clicked, this, &MainWindow::addLayer);
    connect(ui->btn_AddFrame,    &QPushButton::clicked, projectModel, &ProjectModel::addFrame);
    connect(ui->btn_DeleteFrame, &QPushButton::clicked, projectModel, &ProjectModel::deleteCurrentFrame);
    connect(ui->btn_DeleteLayer, &QPushButton::clicked, projectModel, &ProjectModel::deleteCurrentLayer);

    //connect(ui->btn_Paste,  &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::pasteToLayer);
    //connect(ui->btn_Copy,   &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::copyLayer);
    //connect(ui->btn_Cut,    &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::cutLayer);
    connect(ui->btn_Rotate, &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::rotateFloatingImage);

    connect(ui->btn_Save, &QPushButton::clicked, this, &MainWindow::openExportMenu);

    connectPlayerControls();
}

void MainWindow::connectPlayerControls() {
    QPushButton* playBtn = ui->btn_Play;

    connect(playBtn, &QPushButton::clicked, projectModel, &ProjectModel::togglePlay);

    connect(projectModel, &ProjectModel::isPlayingChanged, this, [playBtn](bool playing) {
        QSettings settings("POD2d", "EditorSettings");
        QString theme = settings.value("ui/theme", "dark").toString();

        if (theme == "1bit") {
            playBtn->setIcon(QIcon());
            playBtn->setText(playing ? "X" : ">");
            playBtn->setFont(QFont("Courier New", 14, QFont::Bold));
        } else {
            playBtn->setText("");
            QString basePath = QFileInfo(__FILE__).dir().absolutePath();
            QString iconName = playing ? "stop.png" : "play.png";
            playBtn->setIcon(QIcon(basePath + "/image/" + theme + "/" + iconName));
        }
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

void MainWindow::connectRecentProjects() {
    connect(ui->list_RecentProjects, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        QString filePath = item->data(Qt::UserRole).toString();

        loadProjectFromFile(filePath);
    });
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
    connect(ui->btn_RectangleSelection,     &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Select); });

    connect(ui->slider_BrushSize, QOverload<int>::of(&QSlider::valueChanged), this, &MainWindow::on_spin_brushSize_valueChanged);
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

    ui->act_Clear->setShortcut(QKeySequence("Delete"));

    connect(ui->act_Undo, &QAction::triggered, projectModel, &ProjectModel::undo);
    connect(ui->act_Redo, &QAction::triggered, projectModel, &ProjectModel::redo);

    connect(ui->act_Select, &QAction::triggered, this, &MainWindow::selectAll);
    connect(ui->act_Cut, &QAction::triggered, ui->canvasWidget, &PixelCanvas::cutLayer);
    connect(ui->act_Copy, &QAction::triggered, ui->canvasWidget, &PixelCanvas::copyLayer);
    connect(ui->act_Paste, &QAction::triggered, ui->canvasWidget, &PixelCanvas::pasteToLayer);

    connect(ui->act_Paste, &QAction::triggered, this, &MainWindow::clear);
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

    connect(ui->act_ThemeDark, &QAction::triggered, this, [this]() { applyTheme("dark"); });
    connect(ui->act_ThemeLight, &QAction::triggered, this, [this]() { applyTheme("light"); });
    connect(ui->act_Theme1Bit, &QAction::triggered, this, [this]() { applyTheme("1bit"); });
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

void MainWindow::setupShortcuts() {
    QSettings settings("POD2d", "EditorSettings");

    // Drawing tools
    ui->btn_Pen->setShortcut(QKeySequence(settings.value("shortcuts/pen", "P").toString()));
    ui->btn_Fill->setShortcut(QKeySequence(settings.value("shortcuts/fill", "F").toString()));
    ui->btn_Line->setShortcut(QKeySequence(settings.value("shortcuts/line", "L").toString()));
    ui->btn_Rectangle->setShortcut(QKeySequence(settings.value("shortcuts/rectangle", "R").toString()));
    ui->btn_Circle->setShortcut(QKeySequence(settings.value("shortcuts/circle", "C").toString()));
    ui->btn_Text->setShortcut(QKeySequence(settings.value("shortcuts/text", "T").toString()));
    ui->btn_Dithering->setShortcut(QKeySequence(settings.value("shortcuts/dithering", "D").toString()));
    ui->btn_BrokenLine->setShortcut(QKeySequence(settings.value("shortcuts/brokenLine", "B").toString()));

    // Navigation and selection
    ui->btn_Pan->setShortcut(QKeySequence(settings.value("shortcuts/pan", "Space").toString()));
    ui->btn_RectangleSelection->setShortcut(QKeySequence(settings.value("shortcuts/select", "S").toString()));
    //ui->btn_Clear->setShortcut(QKeySequence(settings.value("shortcuts/clear", "Delete").toString()));********************************

    // Frames and Layers
    ui->btn_AddFrame->setShortcut(QKeySequence(settings.value("shortcuts/addFrame", "Ctrl+N").toString()));
    ui->btn_DeleteFrame->setShortcut(QKeySequence(settings.value("shortcuts/deleteFrame", "Ctrl+Shift+D").toString()));
    ui->btn_AddLayer->setShortcut(QKeySequence(settings.value("shortcuts/addLayer", "Ctrl+Shift+N").toString()));
    ui->btn_DeleteLayer->setShortcut(QKeySequence(settings.value("shortcuts/deleteLayer", "Ctrl+Alt+D").toString()));

    // Animation player
    ui->btn_Play->setShortcut(QKeySequence(settings.value("shortcuts/play", "Enter").toString()));
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

        bool isRGB = dialog.isRGBMode();

        if (projectModel) {
            projectModel->initDefaultProject(projectWidth, projectHeight, isRGB);
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
        saveProject();
    }
}

void MainWindow::openProject() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "Pod2D Project (*.pod2d)"
        );

    if (!path.isEmpty()) {
        loadProjectFromFile(path);
    }
}

void MainWindow::loadProjectFromFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Failed to open the file!");
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    if (!projectModel->loadProjectData(data)) {
        QMessageBox::warning(this, "Error", "The project file is corrupted or has an invalid format!");
        return;
    }

    currentFilePath = path;
    currentProjectName = QFileInfo(path).baseName();
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

    addRecentProject(path);
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


    addRecentProject(currentFilePath);
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

void MainWindow::openPaletteEditor(int colorIndexToEdit) {
    PaletteDialog dialog(this);

    dialog.setPaletteData(customPalette, colorIndexToEdit);

    if (dialog.exec() == QDialog::Accepted) {
        customPalette = dialog.getPalette();
        rebuildPaletteGrid();
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

    setupShortcuts();
}

void MainWindow::addRecentProject(const QString &path) {
    QSettings settings("POD2d", "EditorSettings");
    QStringList recentFiles = settings.value("recentProjects").toStringList();

    recentFiles.removeAll(path);
    recentFiles.prepend(path);

    if (recentFiles.size() > 10) {
        recentFiles.removeLast();
    }

    settings.setValue("recentProjects", recentFiles);
    updateRecentProjectsUI();
}

void MainWindow::loadPalette() {
    QString path = QFileDialog::getOpenFileName(this, "Load Palette", "", "GIMP Palette (*.gpl);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open palette file.");
        return;
    }

    QTextStream in(&file);
    QString header = in.readLine();

    if (!header.startsWith("GIMP Palette")) {
        QMessageBox::warning(this, "Error", "Invalid palette file format. Only .gpl is supported.");
        return;
    }

    QList<QColor> newPalette;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith("#") || line.startsWith("Name:") || line.startsWith("Columns:")) {
            continue;
        }

        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts.size() >= 3) {
            int r = parts[0].toInt();
            int g = parts[1].toInt();
            int b = parts[2].toInt();
            newPalette.append(QColor(r, g, b));
        }
    }

    if (!newPalette.isEmpty()) {
        customPalette = newPalette;
        rebuildPaletteGrid();
    } else {
        QMessageBox::warning(this, "Error", "No colors found in the palette file.");
    }
}

void MainWindow::savePalette() {
    if (customPalette.isEmpty()) return;

    QString path = QFileDialog::getSaveFileName(this, "Save Palette", "my_palette.gpl", "GIMP Palette (*.gpl)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot save palette file.");
        return;
    }

    QTextStream out(&file);

    out << "GIMP Palette\n";
    out << "Name: POD2d_Custom_Palette\n";
    out << "Columns: 4\n";
    out << "# Exported from POD2d\n";

    for (int i = 0; i < customPalette.size(); ++i) {
        const QColor &c = customPalette[i];
        out << c.red() << " " << c.green() << " " << c.blue() << " Color_" << i << "\n";
    }

    file.close();
}

// Group B: UI & State Updates
// ====================================
void MainWindow::rebuildLayersList() {
    QListWidget* layersList = ui->layersListWidget;
    const QSignalBlocker blocker(layersList);
    layersList->clear();

    const int count = projectModel->getLayerCount();

    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

    QString normalItemBorder, selectedItemBorder, itemBg, textColor;
    int borderRadius = (theme == "1bit") ? 0 : 6;

    if (theme == "1bit") {
        normalItemBorder = "border: 1px solid white;";
        selectedItemBorder = "border: 2px solid white;";
        itemBg = "black";
        textColor = "white";
    } else if (theme == "light") {
        normalItemBorder = "border: 1px solid #d4d4d4;";
        selectedItemBorder = "border: 2px solid #0078d7;";
        itemBg = "#e0e0e0";
        textColor = "#202020";
    } else { // dark
        normalItemBorder = "border: 1px solid #444444;";
        selectedItemBorder = "border: 2px solid #888888;";
        itemBg = "#333333";
        textColor = "white";
    }

    layersList->setStyleSheet(
        "QListWidget { outline: 0; background: transparent; border: none; }"
        "QListWidget::item { background-color: " + itemBg + "; " + normalItemBorder + " border-radius: " + QString::number(borderRadius) + "px; margin: 2px; }"
                                                                                                  "QListWidget::item:selected { " + selectedItemBorder + " }"
        );

    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(110, 46));
        layersList->addItem(item);

        QWidget *rowWidget = new QWidget();
        rowWidget->setStyleSheet("background: transparent; border: none;");

        QHBoxLayout *layout = new QHBoxLayout(rowWidget);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(5);

        QLabel *imageLabel = new QLabel();
        imageLabel->setObjectName("layerImage");
        imageLabel->setFixedSize(90, 30);
        imageLabel->setStyleSheet("background-color: transparent; border: none;");
        imageLabel->setAlignment(Qt::AlignCenter);

        QImage thumb = projectModel->getLayerThumbnail(i);
        QPixmap pixmap = QPixmap::fromImage(thumb).scaled(90, 30, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pixmap);

        QLabel *textLabel = new QLabel(QString::number(i));
        textLabel->setFixedSize(20, 30);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setStyleSheet("color: " + textColor + "; font-weight: bold; background: transparent; border: none;");

        layout->addWidget(imageLabel);
        layout->addWidget(textLabel);

        layersList->setItemWidget(item, rowWidget);
    }

    layersList->setCurrentRow(projectModel->getCurrentLayerIndex());
}

void MainWindow::rebuildFramesList() {
    QListWidget* framesList = ui->framesListWidget;
    const QSignalBlocker blocker(framesList);
    framesList->clear();

    const int frameCount = projectModel->getFrameCount();
    framesList->setSpacing(5);

    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

    QString topBorder, normalItemBorder, selectedItemBorder, frameBg, textColor;
    int borderRadius = (theme == "1bit") ? 0 : 6;

    if (theme == "1bit") {
        topBorder = "border-top: 2px solid white;";
        normalItemBorder = "border: 1px solid white;";
        selectedItemBorder = "border: 2px solid white;";
        frameBg = "black";
        textColor = "white";
    } else if (theme == "light") {
        topBorder = "border-top: 1px solid #d4d4d4;";
        normalItemBorder = "border: 1px solid #d4d4d4;";
        selectedItemBorder = "border: 2px solid #0078d7;";
        frameBg = "#e0e0e0";
        textColor = "#202020";
    } else { // dark
        topBorder = "border-top: 2px solid #333333;";
        normalItemBorder = "border: 1px solid #444444;";
        selectedItemBorder = "border: 2px solid #888888;";
        frameBg = "#333333";
        textColor = "white";
    }

    framesList->setStyleSheet(
        "QListWidget { outline: 0; background: transparent; border: none; " + topBorder + " padding-top: 2px; }"
                                                                                          "QListWidget::item { " + normalItemBorder + " border-radius: " + QString::number(borderRadius) + "px; margin: 2px; }"
                                                                                  "QListWidget::item:selected { " + selectedItemBorder + " }"
        );

    for (int i = 0; i < frameCount; ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(150, 96));
        framesList->addItem(item);

        QFrame *frameWidget = new QFrame();
        frameWidget->setStyleSheet(
            "QFrame {"
            "   background-color: " + frameBg + ";"
                        "   border: none;"
                        "   border-radius: " + QString::number(borderRadius) + "px;"
                                              "}"
            );

        QVBoxLayout *layout = new QVBoxLayout(frameWidget);
        layout->setContentsMargins(0, 0, 0, 4);
        layout->setSpacing(2);
        layout->setAlignment(Qt::AlignCenter);

        QLabel *imageLabel = new QLabel();
        imageLabel->setObjectName("frameImage");
        imageLabel->setFixedSize(128, 64);

        imageLabel->setStyleSheet("background-color: transparent; border: none;");
        imageLabel->setAlignment(Qt::AlignCenter);

        QImage thumb = projectModel->getFrameThumbnail(i);
        QPixmap pixmap = QPixmap::fromImage(thumb).scaled(128, 64, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pixmap);

        QLabel *textLabel = new QLabel(QString::number(i + 1));
        textLabel->setAlignment(Qt::AlignCenter);

        textLabel->setStyleSheet("color: " + textColor + "; font-size: 10px; border: none; background: transparent;");

        layout->addWidget(imageLabel);
        layout->addWidget(textLabel);

        framesList->setItemWidget(item, frameWidget);
    }

    framesList->setCurrentRow(projectModel->getCurrentFrameIndex());
}

void MainWindow::rebuildPaletteGrid() {
    QGridLayout *gridLayout = qobject_cast<QGridLayout*>(ui->widget_Palette->layout());

    if (!gridLayout) {
        if (ui->widget_Palette->layout()) {
            delete ui->widget_Palette->layout();
        }
        gridLayout = new QGridLayout(ui->widget_Palette);
    }

    gridLayout->setSpacing(0);

    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

    if (theme == "1bit") {
        gridLayout->setContentsMargins(5, 5, 5, 5);
    } else {
        gridLayout->setContentsMargins(0, 0, 0, 0);
    }

    gridLayout->setAlignment(Qt::AlignTop);

    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    int columns = 4;

    for (int i = 0; i < columns; ++i) {
        gridLayout->setColumnStretch(i, 1);
    }

    QString borderStyle = (theme == "1bit") ? "border: none;" : "border: 1px solid #555;";
    QString borderRadius = (theme == "1bit") ? "border-radius: 0px;" : "border-radius: 2px;";

    for (int i = 0; i < customPalette.size(); ++i) {
        QPushButton *colorBtn = new QPushButton();

        colorBtn->setFixedHeight(24);
        colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        colorBtn->setCursor(Qt::PointingHandCursor);

        colorBtn->setStyleSheet(QString("background-color: %1; %2 %3")
                                    .arg(customPalette[i].name(), borderStyle, borderRadius));

        colorBtn->setProperty("swatchColor", customPalette[i]);
        colorBtn->setProperty("colorIndex", i);

        colorBtn->installEventFilter(this);
        gridLayout->addWidget(colorBtn, i / columns, i % columns);
    }

    updateColorIndicators();
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
    /*
    int baseIconHeight = 32;
    int proportionalIconWidth = (projWidth * baseIconHeight) / projHeight;
    QSize newIconSize(proportionalIconWidth, baseIconHeight);

    //ui->framesListWidget->setIconSize(newIconSize);
    ui->layersListWidget->setIconSize(newIconSize);
    */

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

void MainWindow::updateRecentProjectsUI() {
    ui->list_RecentProjects->clear();

    QSettings settings("POD2d", "EditorSettings");
    QStringList recentFiles = settings.value("recentProjects").toStringList();

    for (const QString &filePath : recentFiles) {
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) continue;

        QListWidgetItem *item = new QListWidgetItem(ui->list_RecentProjects);
        item->setSizeHint(QSize(400, 55));

        QWidget *rowWidget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(rowWidget);
        layout->setContentsMargins(10, 5, 10, 5);
        layout->setSpacing(2);

        QLabel *nameLabel = new QLabel(fileInfo.fileName());
        nameLabel->setStyleSheet("color: #4CAF50; font-weight: bold; font-size: 14px; background: transparent;");

        QLabel *pathLabel = new QLabel(fileInfo.absoluteFilePath());
        pathLabel->setStyleSheet("color: #888888; font-size: 11px; background: transparent;");

        layout->addWidget(nameLabel);
        layout->addWidget(pathLabel);

        ui->list_RecentProjects->setItemWidget(item, rowWidget);

        item->setData(Qt::UserRole, filePath);
    }
}

void MainWindow::updateColorIndicators() {
    QString primaryStyle = QString("background-color: rgba(%1, %2, %3, %4); border: 2px solid white;")
                               .arg(currentPrimaryColor.red()).arg(currentPrimaryColor.green())
                               .arg(currentPrimaryColor.blue()).arg(currentPrimaryColor.alpha());

    QString secondaryStyle = QString("background-color: rgba(%1, %2, %3, %4); border: 2px solid gray;")
                                 .arg(currentSecondaryColor.red()).arg(currentSecondaryColor.green())
                                 .arg(currentSecondaryColor.blue()).arg(currentSecondaryColor.alpha());

    ui->btn_PrimaryColor->setStyleSheet(primaryStyle);
    ui->btn_SecondaryColor->setStyleSheet(secondaryStyle);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->widget_Palette && event->type() == QEvent::MouseButtonDblClick) {
        openPaletteEditor(-1);
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPushButton *btn = qobject_cast<QPushButton*>(watched);

        if (btn && btn->property("swatchColor").isValid()) {

            int colorIndex = btn->property("colorIndex").toInt();

            if (event->type() == QEvent::MouseButtonDblClick) {
                openPaletteEditor(colorIndex);
                return true;
            }

            else if (event->type() == QEvent::MouseButtonPress) {
                QColor clickedColor = btn->property("swatchColor").value<QColor>();

                if (mouseEvent->button() == Qt::MiddleButton) {
                    customPalette.removeAt(colorIndex);
                    rebuildPaletteGrid();
                    return true;
                }
                else if (mouseEvent->button() == Qt::LeftButton) {
                    currentPrimaryColor = clickedColor;
                    ui->canvasWidget->setPrimaryColor(currentPrimaryColor);
                }
                else if (mouseEvent->button() == Qt::RightButton) {
                    currentSecondaryColor = clickedColor;
                    ui->canvasWidget->setSecondaryColor(currentSecondaryColor);
                }

                updateColorIndicators();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::applyTheme(const QString &themeName) {
    QString basePath = QFileInfo(__FILE__).dir().absolutePath();

    QString filePath = QString(basePath + "/themes/%1.qss").arg(themeName);

    QFile file(filePath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        QString styleSheet = stream.readAll();

        styleSheet.replace("{BASE_PATH}", basePath);

        qApp->setStyleSheet(styleSheet);
        file.close();

        QSettings settings("POD2d", "EditorSettings");
        settings.setValue("ui/theme", themeName);
    }

    loadIcons();

    rebuildFramesList();
    rebuildLayersList();
}

QIcon MainWindow::generate1bitIcon(const QString &text) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setPen(Qt::white);

    QFont font("Courier New", 14, QFont::Bold);
    painter.setFont(font);

    painter.drawText(pixmap.rect(), Qt::AlignCenter, text);

    return QIcon(pixmap);
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
    ExportDialog dialog(projectModel, currentProjectName, this);

    dialog.exec();
}
