#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "pixelcanvas.h"
#include "codegenerator.h"
#include "projectmodel.h"
#include "createprojectdialog.h"
#include "exportdialog.h"
#include "settingsdialog.h"
#include "palettedialog.h"
#include "filecontroller.h"
#include "thememanager.h"

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
#include <QSet>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QEvent>
#include <QMenu>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("POD2d");

    initModels();
    setupTheme();
    setupConnections();

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

    ui->layersListWidget->setModel(projectModel);
    ui->framesListWidget->setModel(projectModel);

    fileController = new FileController(projectModel, this, this);

    connect(fileController, &FileController::projectReady, this, &MainWindow::onProjectReady);
    connect(fileController, &FileController::stateChanged, this, &MainWindow::updateWindowTitle);
    connect(fileController, &FileController::recentProjectAdded, this, &MainWindow::addRecentProject);
    connect(fileController, &FileController::requestPasteToLayer, ui->canvasWidget, &PixelCanvas::pasteToLayer);

    this->setWindowTitle("POD2d");
    this->showMaximized();
    setEditorUIEnabled(false);
    setAcceptDrops(true);
    qApp->installEventFilter(this);
}

void MainWindow::setupTheme() {
    QSettings settings("POD2d", "EditorSettings");
    QString currentTheme = settings.value("ui/theme", "dark").toString();

    ThemeManager::applyTheme(currentTheme, this);
    ThemeManager::loadIcons(ui, currentTheme, projectModel, this);
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
    setupPalette();

    setupShortcuts();
}

void MainWindow::connectModelToLists() {
    connectMiniCanvas();
    connect(projectModel, &ProjectModel::imageChanged, this, [this]() { ui->canvasWidget->update(); });
    connect(projectModel, &ProjectModel::projectModified, fileController, &FileController::markModified);

    connect(projectModel, &ProjectModel::activeLayerChanged, this, [this]() { ui->canvasWidget->update(); });
    connect(projectModel, &ProjectModel::frameChanged, this, [this]() {
        ui->canvasWidget->update();
        ui->layersListWidget->rebuildList();
    });
}

void MainWindow::connectMiniCanvas() {
    QLabel* miniCanvas = ui->miniCanvasWidget;

    miniCanvas->setAlignment(Qt::AlignCenter);

    QImage initialImg = projectModel->getFlattenedImage();
    QPixmap initialPixmap = QPixmap::fromImage(initialImg).scaled(
        miniCanvas->size(),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
        );
    miniCanvas->setPixmap(initialPixmap);

    connect(projectModel, &ProjectModel::imageChanged, this, [miniCanvas](const QImage &img) {
        QPixmap pixmap = QPixmap::fromImage(img).scaled(
            miniCanvas->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
            );

        miniCanvas->setPixmap(pixmap);
    });
}

void MainWindow::connectMenuButtons() {
    connect(ui->btn_CreateProject, &QPushButton::clicked, fileController, &FileController::createProject);
    connect(ui->btn_OpenProject,   &QPushButton::clicked, fileController, &FileController::openProject);
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

    connect(ui->btn_HideMiniMap, &QPushButton::clicked, ui->act_ViewMiniMap, &QAction::trigger);
    connect(ui->btn_HideFrames, &QPushButton::clicked, ui->act_ViewFrames, &QAction::trigger);
    connect(ui->btn_HideLayers, &QPushButton::clicked, ui->act_ViewLayers, &QAction::trigger);

    //connect(ui->btn_MergeLayer, &QPushButton::clicked, projectModel, &ProjectModel::mergeLayerDown);
}

void MainWindow::connectPlayerControls() {
    QPushButton* playBtn = ui->btn_Play;

    connect(playBtn, &QPushButton::clicked, projectModel, &ProjectModel::togglePlay);

    connect(projectModel, &ProjectModel::isPlayingChanged, this, [this](bool playing) {
        QSettings settings("POD2d", "EditorSettings");
        QString theme = settings.value("ui/theme", "dark").toString();
        ThemeManager::loadIcons(ui, theme, projectModel, this);
    });

    connect(ui->timeEdit, &QTimeEdit::timeChanged, this, [this](const QTime &time) {
        int msecs = time.msec() + (time.second() * 1000) + (time.minute() * 60000);

        if (msecs < 10) msecs = 10;

        projectModel->setFrameDelay(msecs);
    });

    QTime startTime = ui->timeEdit->time();
    int startMsecs = startTime.msec() + (startTime.second() * 1000) + (startTime.minute() * 60000);
    if (startMsecs < 10) startMsecs = 100;

    projectModel->setFrameDelay(startMsecs);
}

void MainWindow::connectAutoSaveTimer() {
    autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, &QTimer::timeout, this, [this]() {
        if (fileController && fileController->getIsModified() && !fileController->getCurrentFilePath().isEmpty()) {
            fileController->saveProject();
        }
    });
    applySettings();
}

void MainWindow::connectRecentProjects() {
    connect(ui->list_RecentProjects, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        fileController->loadProjectFromFile(item->data(Qt::UserRole).toString());
    });
}

void MainWindow::setupPalette() {
    connect(ui->widget_Palette, &PaletteWidget::primaryColorSelected, this, [this](const QColor &c){
        currentPrimaryColor = c;
        ui->canvasWidget->setPrimaryColor(c);
        updateColorIndicators();
    });

    connect(ui->widget_Palette, &PaletteWidget::secondaryColorSelected, this, [this](const QColor &c){
        currentSecondaryColor = c;
        ui->canvasWidget->setSecondaryColor(c);
        updateColorIndicators();
    });

    connect(ui->widget_Palette, &PaletteWidget::requestEditColor, this, &MainWindow::openPaletteEditor);
}

void MainWindow::connectDrawingTools() {
    connect(ui->btn_Pen,                &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Brush); });
    connect(ui->btn_Eraser,             &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Eraser); });
    connect(ui->btn_Dithering,          &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Dithering); });
    connect(ui->btn_Pipette,            &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Pipette); });
    connect(ui->btn_Line,               &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Line); });
    connect(ui->btn_Rectangle,          &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Rectangle); });
    connect(ui->btn_Circle,             &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Circle); });
    connect(ui->btn_Fill,               &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Fill); });
    connect(ui->btn_BrokenLine,         &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::BrokenLine); });
    connect(ui->btn_Text,               &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Text); });
    connect(ui->btn_Pan,                &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Pan); });
    connect(ui->btn_RectangleSelection, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Select); });
    connect(ui->btn_LassoSelection,     &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::LassoSelect); });
    connect(ui->btn_ShapeSelection,     &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::ShapeSelect); });
    connect(ui->btn_Lighten,            &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Lighten); });

    connect(ui->slider_BrushSize, QOverload<int>::of(&QSlider::valueChanged), this, &MainWindow::on_spin_brushSize_valueChanged);

    connect(ui->canvasWidget, &PixelCanvas::colorPicked, this, [this](const QColor &color, bool isPrimary){
        if (isPrimary) {
            currentPrimaryColor = color;
            ui->canvasWidget->setPrimaryColor(color);
        } else {
            currentSecondaryColor = color;
            ui->canvasWidget->setSecondaryColor(color);
        }
        updateColorIndicators();
    });

    ui->btn_VerticalMiror->setCheckable(true);

    connect(ui->btn_VerticalMiror, &QPushButton::toggled, this, [this](bool checked) {
        ui->canvasWidget->setVerticalMirror(checked);
    });

    connect(ui->canvasWidget, &PixelCanvas::cursorPositionChanged, this, [this](int x, int y) {
        int w = fileController ? fileController->getProjectWidth() : 0;
        int h = fileController ? fileController->getProjectHeight() : 0;

        if (x < 0 || y < 0 || x >= w || y >= h) {
            ui->lbl_Position->setText("Pos - -");
        } else {
            ui->lbl_Position->setText(QString("Pos %1 %2").arg(x).arg(y));
        }
    });

    ui->btn_CenterView->setCheckable(true);

    connect(ui->btn_CenterView, &QPushButton::toggled, this, [this]() {
        ui->canvasWidget->toggleCenterView();
    });

    ui->slider_BrushSize->setToolTip(QString("Brush Size: %1").arg(ui->slider_BrushSize->value()));
}

void MainWindow::connectActions() {
    connectFileActions();
    connectEditActions();
    connectViewActions();
    connectPreferencesActions();
    connectHelpActions();
}

void MainWindow::connectFileActions() {
    connect(ui->act_NewFile, &QAction::triggered, fileController, &FileController::createProject);
    connect(ui->act_Save, &QAction::triggered, fileController, &FileController::saveProject);
    connect(ui->act_SaveAs, &QAction::triggered, fileController, &FileController::saveProjectAs);
    connect(ui->act_ExportCode, &QAction::triggered, this, &MainWindow::openExportMenu);
    connect(ui->act_OpenFile, &QAction::triggered, fileController, &FileController::openProject);
    connect(ui->act_ImportCArray, &QAction::triggered, fileController, &FileController::actionImportCArray);
    connect(ui->act_ImportPNG, &QAction::triggered, fileController, &FileController::actionImportPng);
    connect(ui->act_Close, &QAction::triggered, this, &MainWindow::closeProject);
    connect(ui->act_Exit, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::connectEditActions() {/*
    ui->act_Undo->setShortcut(QKeySequence::Undo);
    ui->act_Redo->setShortcut(QKeySequence::Redo);

    ui->act_Select->setShortcut(QKeySequence::SelectAll);
    ui->act_Cut->setShortcut(QKeySequence::Cut);
    ui->act_Copy->setShortcut(QKeySequence::Copy);
    ui->act_Paste->setShortcut(QKeySequence::Paste);

    ui->act_Clear->setShortcut(QKeySequence("Delete"));*/

    connect(ui->act_Undo, &QAction::triggered, projectModel, &ProjectModel::undo);
    connect(ui->act_Redo, &QAction::triggered, projectModel, &ProjectModel::redo);

    connect(ui->act_Select, &QAction::triggered, this, &MainWindow::selectAll);
    connect(ui->act_Cut, &QAction::triggered, ui->canvasWidget, &PixelCanvas::cutLayer);
    connect(ui->act_Copy, &QAction::triggered, ui->canvasWidget, &PixelCanvas::copyLayer);
    connect(ui->act_Paste, &QAction::triggered, ui->canvasWidget, &PixelCanvas::pasteToLayer);

    connect(ui->act_Clear, &QAction::triggered, this, &MainWindow::clear);
}

void MainWindow::connectViewActions() {
    connect(ui->act_ViewMiniMap, &QAction::triggered, this, [this]() {
        setMiniMapVisible(!ui->miniCanvasFrame->isVisible());
    });

    connect(ui->act_ViewFrames, &QAction::triggered, this, [this]() {
        setFrameListVisible(!ui->framesListWidget->isVisible());
    });

    connect(ui->act_ViewLayers, &QAction::triggered, this, [this]() {
        setLayerListVisible(!ui->layersListWidget->isVisible());
    });

    connect(ui->act_ViewTools, &QAction::triggered, this, [this]() {
        setToolsVisible(!ui->frm_Tools->isVisible());
    });

    connect(ui->act_Palette, &QAction::triggered, this, [this]() {
        setPaletteVisible(!ui->frm_Palette->isVisible());
    });

    connect(ui->act_ThemeDark, &QAction::triggered, this, [this]() {
        ThemeManager::applyTheme("dark", this);
        ThemeManager::loadIcons(ui, "dark", projectModel, this);
        ui->framesListWidget->reloadTheme();
        ui->layersListWidget->reloadTheme();
    });
    connect(ui->act_ThemeLight, &QAction::triggered, this, [this]() {
        ThemeManager::applyTheme("light", this);
        ThemeManager::loadIcons(ui, "light", projectModel, this);
        ui->framesListWidget->reloadTheme();
        ui->layersListWidget->reloadTheme();
    });
    connect(ui->act_Theme1Bit, &QAction::triggered, this, [this]() {
        ThemeManager::applyTheme("1bit", this);
        ThemeManager::loadIcons(ui, "1bit", projectModel, this);
        ui->framesListWidget->reloadTheme();
        ui->layersListWidget->reloadTheme();
    });
}

void MainWindow::connectPreferencesActions(){

    connect(ui->act_Settings, &QAction::triggered, this, [this]() {
        openSettings(0);
    });

    connect(ui->act_KeyBindings, &QAction::triggered, this, [this]() {
        openSettings(3);
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

    auto setShortcutAndToolTip = [&](auto* element, const QString& tooltipName, const QString& settingsKey, const QString& defaultShortcut) {
        if (!element) return;

        QString shortcutStr = settings.value(settingsKey, defaultShortcut).toString();
        QKeySequence sequence(shortcutStr);

        element->setShortcut(sequence);

        if (shortcutStr.isEmpty()) {
            element->setToolTip(tooltipName);
        } else {
            element->setToolTip(QString("%1 (%2)").arg(tooltipName, sequence.toString(QKeySequence::NativeText)));
        }
    };

    // File (QAction)
    setShortcutAndToolTip(ui->act_NewFile, "New Project", "shortcuts/newFile", "Ctrl+N");
    setShortcutAndToolTip(ui->act_Save, "Save", "shortcuts/save", "Ctrl+S");
    setShortcutAndToolTip(ui->act_SaveAs, "Save As", "shortcuts/saveAs", "Ctrl+Shift+S");
    setShortcutAndToolTip(ui->act_OpenFile, "Open", "shortcuts/openFile", "Ctrl+O");
    setShortcutAndToolTip(ui->act_Close, "Close Project", "shortcuts/close", "Ctrl+W");
    setShortcutAndToolTip(ui->act_Exit, "Exit", "shortcuts/exit", "Alt+F4");
    setShortcutAndToolTip(ui->act_ExportCode, "Export", "shortcuts/export", "Ctrl+E");

    // Edit (QAction)
    setShortcutAndToolTip(ui->act_Undo, "Undo", "shortcuts/undo", "Ctrl+Z");
    setShortcutAndToolTip(ui->act_Redo, "Redo", "shortcuts/redo", "Ctrl+Y");
    setShortcutAndToolTip(ui->act_Copy, "Copy", "shortcuts/copy", "Ctrl+C");
    setShortcutAndToolTip(ui->act_Cut, "Cut", "shortcuts/cut", "Ctrl+X");
    setShortcutAndToolTip(ui->act_Paste, "Paste", "shortcuts/paste", "Ctrl+V");
    setShortcutAndToolTip(ui->act_Select, "Select All", "shortcuts/selectAll", "Ctrl+A");
    setShortcutAndToolTip(ui->act_Clear, "Clear Canvas", "shortcuts/clear", "Delete");

    // Drawing tools
    setShortcutAndToolTip(ui->btn_Pen, "Pen Tool", "shortcuts/pen", "P");
    setShortcutAndToolTip(ui->btn_Eraser, "Eraser Tool", "shortcuts/eraser", "E");
    setShortcutAndToolTip(ui->btn_Fill, "Fill Tool", "shortcuts/fill", "F");
    setShortcutAndToolTip(ui->btn_Line, "Line Tool", "shortcuts/line", "L");
    setShortcutAndToolTip(ui->btn_Rectangle, "Rectangle", "shortcuts/rectangle", "R");
    setShortcutAndToolTip(ui->btn_Circle, "Circle", "shortcuts/circle", "C");
    setShortcutAndToolTip(ui->btn_Text, "Text Tool", "shortcuts/text", "T");
    setShortcutAndToolTip(ui->btn_Dithering, "Dithering Tool", "shortcuts/dithering", "D");
    setShortcutAndToolTip(ui->btn_BrokenLine, "Broken Line Tool", "shortcuts/brokenLine", "B");

    // Navigation and selection
    setShortcutAndToolTip(ui->btn_Pan, "Pan Tool", "shortcuts/pan", "Space");
    setShortcutAndToolTip(ui->btn_LassoSelection, "Lasso Selection", "shortcuts/lassoSelection", "S");
    setShortcutAndToolTip(ui->btn_RectangleSelection, "Selection", "shortcuts/select", "");
    setShortcutAndToolTip(ui->btn_ShapeSelection, "Shape Selection", "shortcuts/shapeSelection", "");

    // Modifiers & Views
    setShortcutAndToolTip(ui->btn_Rotate, "Rotate", "shortcuts/rotate", "");
    setShortcutAndToolTip(ui->btn_Lighten, "Lighten", "shortcuts/lighten", "");
    setShortcutAndToolTip(ui->btn_VerticalMiror, "Vertical Miror", "shortcuts/verticalMiror", "");
    setShortcutAndToolTip(ui->btn_CenterView, "Center View", "shortcuts/centerView", "");

    // Frames and Layers
    setShortcutAndToolTip(ui->btn_AddFrame, "Add Frame", "shortcuts/addFrame", "Ctrl+N");
    setShortcutAndToolTip(ui->btn_DeleteFrame, "Delete Frame", "shortcuts/deleteFrame", "Ctrl+Shift+D");
    setShortcutAndToolTip(ui->btn_AddLayer, "Add Layer", "shortcuts/addLayer", "Ctrl+Shift+N");
    setShortcutAndToolTip(ui->btn_DeleteLayer, "Delete Layer", "shortcuts/deleteLayer", "Ctrl+Alt+D");

    // Player & UI Toggles
    setShortcutAndToolTip(ui->btn_Play, "Play Animation", "shortcuts/play", "Return");
    setShortcutAndToolTip(ui->btn_HideMiniMap, "Mini Map", "shortcuts/miniMap", "");
    setShortcutAndToolTip(ui->btn_HideFrames, "Frames", "shortcuts/hideFrames", "");
    setShortcutAndToolTip(ui->btn_HideLayers, "Layers", "shortcuts/hideLayers", "");
    setShortcutAndToolTip(ui->btn_EditMode, "Edit Mode", "shortcuts/editMode", "");

    // Actions (QAction)
    setShortcutAndToolTip(ui->act_Clear, "Clear", "shortcuts/clear", "Delete");
    setShortcutAndToolTip(ui->act_Undo, "Undo", "shortcuts/undo", "Ctrl+Z");
    setShortcutAndToolTip(ui->act_Redo, "Redo", "shortcuts/redo", "Ctrl+Y");
    setShortcutAndToolTip(ui->act_Copy, "Copy", "shortcuts/copy", "Ctrl+C");
    setShortcutAndToolTip(ui->act_Cut, "Cut", "shortcuts/cut", "Ctrl+X");
    setShortcutAndToolTip(ui->act_Paste, "Paste", "shortcuts/paste", "Ctrl+V");
    setShortcutAndToolTip(ui->act_ExportCode, "Export", "shortcuts/export", "Ctrl+E");
}

// ==========================================
// 3. Logic methods and handlers
// ==========================================

// Group A: File & Project Management
// ====================================
void MainWindow::onProjectReady(int width, int height, bool isRgb) {
    ui->lbl_WidthHeight->setText("[" + QString::number(width) + "x" + QString::number(height) + "]");

    ui->canvasWidget->setCanvasSize(width, height);
    ui->canvasWidget->resetToolState();
    ui->canvasWidget->setTool(DrawTool::Brush);
    ui->slider_BrushSize->setValue(1);
    ui->canvasWidget->setBrushSize(1);

    currentPrimaryColor = Qt::white;
    currentSecondaryColor = Qt::black;
    ui->canvasWidget->setPrimaryColor(currentPrimaryColor);
    ui->canvasWidget->setSecondaryColor(currentSecondaryColor);
    updateColorIndicators();

    ui->layersListWidget->rebuildList();
    ui->framesListWidget->rebuildList();
    updateUIProportions(width, height);

    setEditorUIEnabled(true);

    QSettings settings("POD2d", "EditorSettings");
    setLayerListVisible(settings.value("ui/showLayers", true).toBool());
    setFrameListVisible(settings.value("ui/showFrames", true).toBool());
    setToolsVisible(settings.value("ui/showTools", true).toBool());
    setMiniMapVisible(settings.value("ui/showMiniMap", true).toBool());

    ui->act_Palette->setEnabled(isRgb);
    ui->frm_Palette->setVisible(isRgb);
    ui->btn_Pipette->setVisible(isRgb);

    ui->stackedWidget->setCurrentIndex(1);

    QTimer::singleShot(50, this, [this]() {
        ui->canvasWidget->fitToScreen();
        ui->canvasWidget->update();
    });
}

void MainWindow::updateWindowTitle() {
    QString titleName = fileController->getCurrentProjectName().isEmpty() ? "Untitled" : fileController->getCurrentProjectName();
    if (fileController->getIsModified()) {
        titleName += "*";
    }
    this->setWindowTitle("POD2d - " + titleName);
}

void MainWindow::openFile(const QString &filePath) {
    if (!fileController) return;

    if (filePath.endsWith(".png", Qt::CaseInsensitive)) {
        fileController->openPngAsProject(filePath);
    } else {
        fileController->loadProjectFromFile(filePath);
    }
}

void MainWindow::closeProject() {
    if (!maybeSave()) return;
    ui->stackedWidget->setCurrentIndex(0);
    setEditorUIEnabled(false);

    if (fileController) fileController->resetState();

    this->setWindowTitle("POD2d");
    if (projectModel) {
        ui->canvasWidget->resetToolState();
        ui->canvasWidget->update();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        QSettings settings("POD2d", "EditorSettings");
        settings.setValue("ui/showLayers", ui->layersListWidget->isVisible());
        settings.setValue("ui/showFrames", ui->framesListWidget->isVisible());
        settings.setValue("ui/showTools", ui->frm_Tools->isVisible());
        settings.setValue("ui/showMiniMap", ui->miniCanvasFrame->isVisible());

        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::maybeSave() {
    if (ui->stackedWidget->currentIndex() == 0 || !fileController->getIsModified()) return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(this, "POD2d",
                                                           "You have unsaved changes. Do you want to save them before exiting?",
                                                           QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        fileController->saveProject();
        return true;
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void MainWindow::openSettings(int tabIndex) {
    SettingsDialog dialog(this);

    dialog.setActiveTab(tabIndex);

    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);

    if (dialog.exec() == QDialog::Accepted) {
        applySettings();
    }
}

void MainWindow::openPaletteEditor(int colorIndexToEdit) {
    PaletteDialog dialog(this);

    dialog.setPaletteData(ui->widget_Palette->getPalette(), colorIndexToEdit);

    if (dialog.exec() == QDialog::Accepted) {
        ui->widget_Palette->setPalette(dialog.getPalette());
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

    bool rightEraser = settings.value("editor/rightClickEraser", false).toBool();
    ui->canvasWidget->setEraserOnRightClick(rightEraser);

    projectModel->loadSettings();
    setupShortcuts();

    int scale = settings.value("ui/scale", 100).toInt();
    QFont f = qApp->font();
    f.setPointSize(9 * scale / 100);
    qApp->setFont(f);

    QString theme = settings.value("ui/theme", "dark").toString();
    ThemeManager::applyTheme(theme, this);
    ThemeManager::loadIcons(ui, theme, projectModel, this);

    bool showGrid = settings.value("canvas/showGrid", true).toBool();
    QString gridColor = settings.value("canvas/gridColor", "#333333").toString();
    QString bgStyle = settings.value("canvas/bgStyle", "Checkerboard").toString();

    ui->canvasWidget->setShowGrid(showGrid);
    ui->canvasWidget->setGridColor(QColor(gridColor));
    ui->canvasWidget->setBackgroundStyle(bgStyle);
    ui->canvasWidget->update();
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
        ui->widget_Palette->setPalette(newPalette);
    } else {
        QMessageBox::warning(this, "Error", "No colors found in the palette file.");
    }
}

void MainWindow::savePalette() {
    QList<QColor> currentPalette = ui->widget_Palette->getPalette();

    if (currentPalette.isEmpty()) return;

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

    for (int i = 0; i < currentPalette.size(); ++i) {
        const QColor &c = currentPalette[i];
        out << c.red() << " " << c.green() << " " << c.blue() << " Color_" << i << "\n";
    }

    file.close();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (urls.first().isLocalFile()) {
            QString filePath = urls.first().toLocalFile();

            if (filePath.endsWith(".png", Qt::CaseInsensitive) ||
                filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
                filePath.endsWith(".pod2d", Qt::CaseInsensitive)) {

                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event) {
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;
    QString filePath = urls.first().toLocalFile();
    if (filePath.endsWith(".pod2d", Qt::CaseInsensitive)) {
        fileController->loadProjectFromFile(filePath);
    } else {
        fileController->importPngToCanvas(filePath);
    }
    event->acceptProposedAction();
}

// Group B: UI & State Updates
// ====================================
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
    ui->act_Clear->setEnabled(enabled);

    ui->act_ViewMiniMap->setEnabled(enabled);
    ui->act_ViewFrames->setEnabled(enabled);
    ui->act_ViewLayers->setEnabled(enabled);
    ui->act_ViewTools->setEnabled(enabled);
    ui->act_Palette->setEnabled(enabled);
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

    bool isVisible = ui->frm_Palette->isVisible();

    if(!isVisible) {
        ui->rightPanelFrame->setVisible(visible);
    }
}

void MainWindow::setToolsVisible(bool visible) {
    ui->frm_Tools->setVisible(visible);
    ui->miniCanvasFrame->setVisible(visible);
    ui->toolsLabel->setVisible(visible);

    QString text = visible ? "Hide Tools" : "Show Tools";
    ui->act_ViewTools->setText(text);
    ui->act_ViewMiniMap->setEnabled(visible);
}

void MainWindow::setPaletteVisible(bool visible) {
    ui->frm_Palette->setVisible(visible);

    QString text = visible ? "Hide Palette" : "Show Palette";
    ui->act_Palette->setText(text);

    bool isVisible = ui->layersListWidget->isVisible();

    if(!isVisible) {
        ui->rightPanelFrame->setVisible(visible);
    }

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

    ui->framesListWidget->reloadTheme();
    ui->layersListWidget->rebuildList();

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

    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *clickedWidget = qobject_cast<QWidget*>(watched);

        bool isModifierButton = (clickedWidget == ui->btn_Rotate ||
                                 clickedWidget == ui->btn_VerticalMiror);

        if (clickedWidget && clickedWidget != ui->canvasWidget && !isModifierButton) {
            ui->canvasWidget->commitFloatingImage();
        }
    }

    return QMainWindow::eventFilter(watched, event);
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

void MainWindow::on_spin_brushSize_valueChanged(int value) {
    ui->canvasWidget->setBrushSize(value);
    ui->slider_BrushSize->setToolTip(QString("Brush Size: %1").arg(value));
}

void MainWindow::chooseAndSetColor() {
    const QColor selectedColor = QColorDialog::getColor(ui->canvasWidget->getMonoDisplayColor(), this, "Choose OLED Color");

    if (selectedColor.isValid()) {
        ui->canvasWidget->setMonoDisplayColor(selectedColor);

        ui->canvasWidget->setPrimaryColor(selectedColor);
        currentPrimaryColor = selectedColor;
        updateColorIndicators();
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
    ExportDialog dialog(projectModel, fileController->getCurrentProjectName(), this);
    dialog.exec();
}
