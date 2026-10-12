#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "pixelcanvas.h"
#include "projectmodel.h"
#include "exportdialog.h"
#include "settingsdialog.h"
#include "palettedialog.h"
#include "filecontroller.h"
#include "thememanager.h"
#include "actionmanager.h"
#include "settingsmanager.h"
#include "flowlayout.h"

#include <QFileDialog>
#include <QColorDialog>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QClipboard>
#include <QGuiApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QTextStream>
#include <QRegularExpression>
#include <QFile>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QEvent>
#include <QDockWidget>
#include <QAction>
#include <QSettings>
#include <QToolBar>
#include <QTabBar>
#include <QMouseEvent>

namespace {
bool shapeWantsHorizontal(QMainWindow *mw, QDockWidget *dock, bool currentHorizontal) {
    if (dock->isVisible() && dock->width() > 0 && dock->height() > 0) {
        const double ratio = double(dock->width()) / double(dock->height());
        if (ratio > 1.15)       return true;
        if (ratio < 1.0 / 1.15) return false;
        return currentHorizontal;
    }
    const Qt::DockWidgetArea area = mw->dockWidgetArea(dock);
    return dock->isFloating()
           || area == Qt::BottomDockWidgetArea
           || area == Qt::TopDockWidgetArea
           || area == Qt::NoDockWidgetArea;
}

constexpr int kDockStateVersion = 2;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("POD2d");

    setupDocks();
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
// 0. Dock widgets
// ==========================================

void MainWindow::setupToolsFlow() {
    auto *flow = new FlowLayout(ui->toolButtonsHost, 0, 0, 0);
    flow->setGroupGap(10);
    flow->setPreferredWidth(147);

    const QList<QList<QPushButton *>> groups = {
        { ui->btn_Pen, ui->btn_Eraser, ui->btn_Text, ui->btn_Fill, ui->btn_Dithering, ui->btn_Pipette },
        { ui->btn_Line, ui->btn_BrokenLine, ui->btn_Circle, ui->btn_Rectangle },
        { ui->btn_RectangleSelection, ui->btn_LassoSelection, ui->btn_ShapeSelection,
         ui->btn_Pan, ui->btn_Rotate, ui->btn_Lighten }
    };
    for (const auto &group : groups) {
        for (QPushButton *b : group) flow->addWidget(b);
        flow->addBreak();
    }
}

void MainWindow::refreshMiniMap() {
    if (!projectModel) return;
    const QImage img = projectModel->getFlattenedImage();
    if (img.isNull()) return;

    QSize target = ui->miniCanvasWidget->size();
    if (target.width() < 40 || target.height() < 30) target = QSize(140, 90);

    ui->miniCanvasWidget->setPixmap(
        QPixmap::fromImage(projectModel->applyBackground(img))
            .scaled(target, Qt::KeepAspectRatio, Qt::FastTransformation));
}

void MainWindow::setupStatusBar() {
    ui->statusbar->setSizeGripEnabled(false);
    ui->statusbar->addWidget(ui->lbl_Position);

    auto *sep = new QFrame(this);
    sep->setObjectName("statusSep");
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedHeight(14);
    ui->statusbar->addWidget(sep);

    ui->statusbar->addWidget(ui->lbl_WidthHeight);
}

void MainWindow::setupDocks() {
    setupToolsFlow();
    setupStatusBar();

    ui->canvasWidget->setAttribute(Qt::WA_StyledBackground, true);

    m_docks = { ui->dockMiniMap, ui->dockTools, ui->dockLayers,
               ui->dockPalette, ui->dockFrames };

    setDockOptions(QMainWindow::AnimatedDocks
                   | QMainWindow::AllowTabbedDocks
                   | QMainWindow::AllowNestedDocks);
    setDockNestingEnabled(true);
    setCorner(Qt::BottomLeftCorner,  Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    toolBarMain = new QToolBar(tr("Main"), this);
    toolBarMain->setObjectName("toolBarMain");
    toolBarMain->setMovable(false);
    toolBarMain->setFloatable(false);
    toolBarMain->setAllowedAreas(Qt::TopToolBarArea);
    toolBarMain->setContextMenuPolicy(Qt::PreventContextMenu);
    ui->frm_Menu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    toolBarMain->addWidget(ui->frm_Menu);
    addToolBar(Qt::TopToolBarArea, toolBarMain);

    QAction *anchor = ui->menuView->actions().value(0);
    for (QDockWidget *d : m_docks)
        ui->menuView->insertAction(anchor, d->toggleViewAction());
    ui->menuView->insertSeparator(anchor);
    actResetLayout = new QAction(tr("Reset Layout"), this);
    connect(actResetLayout, &QAction::triggered, this, &MainWindow::resetDockLayout);
    ui->menuView->insertAction(anchor, actResetLayout);

    connect(ui->dockFrames, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) { updateFramesOrientation(); });
    connect(ui->dockFrames, &QDockWidget::topLevelChanged,     this, [this](bool)               { updateFramesOrientation(); });
    connect(ui->dockLayers, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) { updateLayersOrientation(); });
    connect(ui->dockLayers, &QDockWidget::topLevelChanged,     this, [this](bool)               { updateLayersOrientation(); });
    connect(ui->dockTools,  &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea) { QTimer::singleShot(0, this, &MainWindow::snapToolsWidth); });

    m_defaultDockState = saveState(kDockStateVersion);
    updateFramesOrientation();
    updateLayersOrientation();

    for (QDockWidget *d : m_docks) d->hide();
    toolBarMain->hide();
    ui->statusbar->hide();
}

void MainWindow::updateFramesOrientation() {
    const bool current = ui->framesLayout->direction() == QBoxLayout::LeftToRight;
    const bool horizontal = shapeWantsHorizontal(this, ui->dockFrames, current);

    ui->framesListWidget->setHorizontal(horizontal);
    ui->framesLayout->setDirection(horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    ui->frm_AddDeleteFrame->setMaximumWidth(horizontal ? 160 : QWIDGETSIZE_MAX);
}

void MainWindow::updateLayersOrientation() {
    const bool current = ui->layersLayout->direction() == QBoxLayout::LeftToRight;
    const bool horizontal = shapeWantsHorizontal(this, ui->dockLayers, current);

    ui->layersListWidget->setHorizontal(horizontal);

    ui->layersLayout->setDirection(horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    ui->addDeleteLayerLayout->setDirection(horizontal ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);

    QFrame *bar = ui->frm_AddDeleteLayer;
    if (horizontal) {
        bar->setMinimumSize(70, 0);
        bar->setMaximumSize(90, QWIDGETSIZE_MAX);
    } else {
        bar->setMinimumSize(0, 40);
        bar->setMaximumSize(QWIDGETSIZE_MAX, 40);
    }
}

void MainWindow::snapToolsWidth() {
    if (property("snappingTools").toBool()) return;
    if (QGuiApplication::mouseButtons() & Qt::LeftButton) return;

    QDockWidget *dock = ui->dockTools;
    if (!dock->isVisible() || dock->isFloating()) return;

    const Qt::DockWidgetArea area = dockWidgetArea(dock);
    if (area != Qt::LeftDockWidgetArea && area != Qt::RightDockWidgetArea) return;

    constexpr int kCell = 49;
    constexpr int kMargin = 2;
    constexpr int kMaxCols = 6;

    const int viewportW = ui->toolsScroll->viewport()->width();
    if (viewportW <= 0) return;

    const int cols = qBound(1, qRound(double(viewportW - 2 * kMargin) / kCell), kMaxCols);
    const int delta = viewportW - (cols * kCell + 2 * kMargin);
    if (delta == 0) return;

    setProperty("snappingTools", true);
    resizeDocks({ dock }, { dock->width() - delta }, Qt::Horizontal);
    setProperty("snappingTools", false);
}

void MainWindow::saveDockVisibility() {
    SettingsManager::setShowLayers (ui->dockLayers ->toggleViewAction()->isChecked());
    SettingsManager::setShowFrames (ui->dockFrames ->toggleViewAction()->isChecked());
    SettingsManager::setShowTools  (ui->dockTools  ->toggleViewAction()->isChecked());
    SettingsManager::setShowMiniMap(ui->dockMiniMap->toggleViewAction()->isChecked());
}

void MainWindow::saveDockState() {
    QSettings s;
    s.setValue("ui/dockState", saveState(kDockStateVersion));
}

void MainWindow::restoreDockState() {
    QSettings s;
    const QByteArray st = s.value("ui/dockState").toByteArray();
    if (!st.isEmpty()) restoreState(st, kDockStateVersion);
}

void MainWindow::resetDockLayout() {
    restoreState(m_defaultDockState, kDockStateVersion);
    for (QDockWidget *d : m_docks) {
        d->setFloating(false);
        d->show();
    }
    updateFramesOrientation();
    updateLayersOrientation();
    if (!ui->dockPalette->toggleViewAction()->isEnabled())
        ui->dockPalette->hide();
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
    QString currentTheme = SettingsManager::getTheme();

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

    ActionManager::setupShortcuts(ui);
}

void MainWindow::connectModelToLists() {
    connectMiniCanvas();
    connect(projectModel, &ProjectModel::imageChanged, this, [this]() { ui->canvasWidget->update(); });
    connect(projectModel, &ProjectModel::projectModified, fileController, &FileController::markModified);

    connect(projectModel, &ProjectModel::activeLayerChanged, this, [this]() {
        ui->canvasWidget->update();
        updateUiStates();
    });

    connect(projectModel, &ProjectModel::frameChanged, this, [this]() {
        ui->canvasWidget->update();
        ui->layersListWidget->rebuildList();
    });

    connect(projectModel, &ProjectModel::framesListChanged, this, &MainWindow::updateUiStates);
    connect(projectModel, &ProjectModel::isPlayingChanged, this, [this](bool) { updateUiStates(); });
}

void MainWindow::connectMiniCanvas() {
    QLabel* miniCanvas = ui->miniCanvasWidget;

    miniCanvas->setAlignment(Qt::AlignCenter);

    QImage initialImg = projectModel->applyBackground(projectModel->getFlattenedImage());
    QPixmap initialPixmap = QPixmap::fromImage(initialImg).scaled(
        QSize(140, 90).expandedTo(miniCanvas->size().boundedTo(QSize(400, 300))),
        Qt::KeepAspectRatio,
        Qt::FastTransformation
        );
    miniCanvas->setPixmap(initialPixmap);

    connect(projectModel, &ProjectModel::imageChanged, this, [miniCanvas, this](const QImage &img) {
        QImage bgImg = projectModel->applyBackground(img);
        QPixmap pixmap = QPixmap::fromImage(bgImg).scaled(
            miniCanvas->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
            );

        miniCanvas->setPixmap(pixmap);
    });
}

void MainWindow::connectMenuButtons() {
    connect(ui->btn_CreateProject, &QPushButton::clicked, this, [this]() {
        if (maybeSave()) fileController->createProject();
    });
    connect(ui->btn_OpenProject, &QPushButton::clicked, this, [this]() {
        if (maybeSave()) fileController->openProject();
    });
}

void MainWindow::connectEditorControls() {
    connect(ui->btn_Undo,  &QPushButton::clicked, this, &MainWindow::undo);
    connect(ui->btn_Redo,  &QPushButton::clicked, this, &MainWindow::redo);

    connect(projectModel, &ProjectModel::canUndoChanged, this, [this](bool can) {
        QTimer::singleShot(0, this, [this, can]() {
            ui->btn_Undo->setEnabled(can);
            ui->act_Undo->setEnabled(can);
        });
    });
    connect(projectModel, &ProjectModel::canRedoChanged, this, [this](bool can) {
        QTimer::singleShot(0, this, [this, can]() {
            ui->btn_Redo->setEnabled(can);
            ui->act_Redo->setEnabled(can);
        });
    });

    connect(ui->btn_AddLayer,    &QPushButton::clicked, projectModel, &ProjectModel::addLayer);
    connect(ui->btn_AddFrame,    &QPushButton::clicked, projectModel, &ProjectModel::addFrame);
    connect(ui->btn_DeleteFrame, &QPushButton::clicked, projectModel, &ProjectModel::deleteCurrentFrame);
    connect(ui->btn_DeleteLayer, &QPushButton::clicked, projectModel, &ProjectModel::deleteCurrentLayer);

    connect(ui->btn_Rotate, &QPushButton::clicked, ui->canvasWidget, &PixelCanvas::rotateFloatingImage);

    connect(ui->btn_Save, &QPushButton::clicked, this, &MainWindow::openExportMenu);

    connectPlayerControls();

    connect(ui->btn_HideMiniMap, &QPushButton::clicked, ui->dockMiniMap->toggleViewAction(), &QAction::trigger);
    connect(ui->btn_HideFrames,  &QPushButton::clicked, ui->dockFrames->toggleViewAction(),  &QAction::trigger);
    connect(ui->btn_HideLayers,  &QPushButton::clicked, ui->dockLayers->toggleViewAction(),  &QAction::trigger);
}

void MainWindow::connectPlayerControls() {
    QPushButton* playBtn = ui->btn_Play;

    connect(playBtn, &QPushButton::clicked, projectModel, &ProjectModel::togglePlay);

    connect(projectModel, &ProjectModel::isPlayingChanged, this, [this](bool playing) {
        QString theme = SettingsManager::getTheme();
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
        openFile(item->data(Qt::UserRole).toString());
    });
}

void MainWindow::setupPalette() {
    connect(ui->widget_Palette, &PaletteWidget::primaryColorSelected, this, [this](const QColor &c){
        currentPrimaryColor = c;
        ui->canvasWidget->setPrimaryColor(c);
        updateColorIndicators();

        DrawTool tool = ui->canvasWidget->getCurrentTool();
        if (tool == DrawTool::Eraser || tool == DrawTool::Pan ||
            tool == DrawTool::Pipette || tool == DrawTool::Select ||
            tool == DrawTool::LassoSelect || tool == DrawTool::ShapeSelect ||
            tool == DrawTool::Lighten) {
            ui->btn_Pen->click();
        }
    });

    connect(ui->widget_Palette, &PaletteWidget::secondaryColorSelected, this, [this](const QColor &c){
        currentSecondaryColor = c;
        ui->canvasWidget->setSecondaryColor(c);
        updateColorIndicators();

        DrawTool tool = ui->canvasWidget->getCurrentTool();
        if (tool == DrawTool::Eraser || tool == DrawTool::Pan ||
            tool == DrawTool::Pipette || tool == DrawTool::Select ||
            tool == DrawTool::LassoSelect || tool == DrawTool::ShapeSelect ||
            tool == DrawTool::Lighten) {
            ui->btn_Pen->click();
        }
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

        ui->btn_Pen->click();
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

    ui->slider_BrushSize->setToolTip(tr("Brush Size: %1").arg(ui->slider_BrushSize->value()));
}

void MainWindow::connectActions() {
    connectFileActions();
    connectEditActions();
    connectViewActions();
    connectPreferencesActions();
    connectHelpActions();
}

void MainWindow::connectFileActions() {
    connect(ui->act_NewFile, &QAction::triggered, this, [this]() {
        if (maybeSave()) fileController->createProject();
    });
    connect(ui->act_OpenFile, &QAction::triggered, this, [this]() {
        if (maybeSave()) fileController->openProject();
    });
    connect(ui->act_ImportCArray, &QAction::triggered, this, [this]() {
        if (maybeSave()) fileController->actionImportCArray();
    });
    connect(ui->act_ImportPNG, &QAction::triggered, this, [this]() {
        if (maybeSave()) fileController->actionImportPng();
    });

    connect(ui->act_Save, &QAction::triggered, fileController, &FileController::saveProject);
    connect(ui->act_SaveAs, &QAction::triggered, fileController, &FileController::saveProjectAs);
    connect(ui->act_ExportCode, &QAction::triggered, this, &MainWindow::openExportMenu);
    connect(ui->act_RenameFile, &QAction::triggered, fileController, &FileController::renameProject);
    connect(ui->act_Close, &QAction::triggered, this, &MainWindow::closeProject);
    connect(ui->act_Exit, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::connectEditActions() {
    connect(ui->act_Undo, &QAction::triggered, this, &MainWindow::undo);
    connect(ui->act_Redo, &QAction::triggered, this, &MainWindow::redo);

    connect(ui->act_Select, &QAction::triggered, this, &MainWindow::selectAll);
    connect(ui->act_Cut, &QAction::triggered, ui->canvasWidget, &PixelCanvas::cutLayer);
    connect(ui->act_Copy, &QAction::triggered, ui->canvasWidget, &PixelCanvas::copyLayer);
    connect(ui->act_Paste, &QAction::triggered, ui->canvasWidget, &PixelCanvas::pasteToLayer);

    connect(ui->act_Clear, &QAction::triggered, this, &MainWindow::clear);
}

void MainWindow::connectViewActions() {

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
        QDesktopServices::openUrl(QUrl("https://github.com/ilertwo/POD2d/tree/release"));
        //QMessageBox::information(this, tr("Check for Updates"), tr("You are using the latest version of POD2d."));
    });

    connect(ui->act_HelpAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About POD2d"),
                           tr("<b>POD2d</b> - Pixel OLED Designer<br><br>"
                              "Version: 1.0.0<br>"
                              "Author: Ilertwo<br><br>"
                              "Created using C++ and Qt."));
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

    QColor savedMono = SettingsManager::getMonoColor();
    ui->canvasWidget->setMonoDisplayColor(savedMono);
    projectModel->setMonoColor(savedMono);

    ui->layersListWidget->rebuildList();
    ui->framesListWidget->rebuildList();
    updateUIProportions(width, height);

    setEditorUIEnabled(true);
    updateUiStates();

    if (!property("dockStateApplied").toBool()) {
        restoreDockState();
        setProperty("dockStateApplied", true);
        updateFramesOrientation();
        updateLayersOrientation();
    }

    setLayerListVisible(SettingsManager::getShowLayers());
    setFrameListVisible(SettingsManager::getShowFrames());
    setToolsVisible(SettingsManager::getShowTools());
    setMiniMapVisible(SettingsManager::getShowMiniMap());

    ui->dockPalette->toggleViewAction()->setEnabled(isRgb);
    setPaletteVisible(isRgb);
    ui->btn_Pipette->setVisible(isRgb);

    toolBarMain->show();
    ui->statusbar->show();
    ui->stackedWidget->setCurrentIndex(1);
    QTimer::singleShot(150, this, &MainWindow::snapToolsWidth);

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

    saveDockVisibility();
    saveDockState();
    for (QDockWidget *d : m_docks) d->hide();
    toolBarMain->hide();
    ui->statusbar->hide();

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
        if (ui->stackedWidget->currentIndex() == 1) {
            saveDockVisibility();
            saveDockState();
        }

        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::maybeSave() {
    if (ui->stackedWidget->currentIndex() == 0 || !fileController->getIsModified()) return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("POD2d"),
                                                           tr("You have unsaved changes. Do you want to save them before exiting?"),
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
    if (SettingsManager::getAutoSave()) {
        autoSaveTimer->start(SettingsManager::getAutoSaveInterval() * 60 * 1000);
    } else {
        autoSaveTimer->stop();
    }

    ui->canvasWidget->setEraserOnRightClick(SettingsManager::getRightClickEraser());

    projectModel->loadSettings();
    ActionManager::setupShortcuts(ui);

    QFont f = qApp->font();
    f.setPointSize(9 * SettingsManager::getScale() / 100);
    qApp->setFont(f);

    QString theme = SettingsManager::getTheme();
    ThemeManager::applyTheme(theme, this);
    ThemeManager::loadIcons(ui, theme, projectModel, this);

    ui->canvasWidget->setShowGrid(SettingsManager::getShowGrid());
    ui->canvasWidget->setGridColor(SettingsManager::getGridColor());
    ui->canvasWidget->setBackgroundStyle(SettingsManager::getBgStyle());
    ui->canvasWidget->update();

    if (projectModel && ui->stackedWidget->currentIndex() != 0) {
        projectModel->notifyImageChanged();
        ui->layersListWidget->rebuildList();
        ui->framesListWidget->rebuildList();
    }
}

void MainWindow::addRecentProject(const QString &path) {
    SettingsManager::addRecentProject(path);
    updateRecentProjectsUI();
}

void MainWindow::loadPalette() {
    QString path = QFileDialog::getOpenFileName(this, tr("Load Palette"), "", tr("GIMP Palette (*.gpl);;All Files (*)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open palette file."));
        return;
    }

    QTextStream in(&file);
    QString header = in.readLine();

    if (!header.startsWith("GIMP Palette")) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid palette file format. Only .gpl is supported."));
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
        QMessageBox::warning(this, tr("Error"), tr("No colors found in the palette file."));
    }
}

void MainWindow::savePalette() {
    QList<QColor> currentPalette = ui->widget_Palette->getPalette();
    if (currentPalette.isEmpty()) return;

    QString path = QFileDialog::getSaveFileName(this, tr("Save Palette"), "my_palette.gpl", tr("GIMP Palette (*.gpl)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot save palette file."));
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

    for (QDockWidget *d : m_docks)
        d->toggleViewAction()->setEnabled(enabled);
    if (actResetLayout) actResetLayout->setEnabled(enabled);
}

void MainWindow::setMiniMapVisible(bool visible) {
    ui->dockMiniMap->setVisible(visible);
}

void MainWindow::setFrameListVisible(bool visible) {
    ui->dockFrames->setVisible(visible);
}

void MainWindow::setLayerListVisible(bool visible) {
    ui->dockLayers->setVisible(visible);
}

void MainWindow::setToolsVisible(bool visible) {
    ui->dockTools->setVisible(visible);
}

void MainWindow::setPaletteVisible(bool visible) {
    ui->dockPalette->setVisible(visible);
}

void MainWindow::updateUIProportions(int projWidth, int projHeight) {
    if (projHeight == 0) return;

    ui->framesListWidget->reloadTheme();
    ui->layersListWidget->rebuildList();

    QImage currentImg = projectModel->getFlattenedImage();
    if (!currentImg.isNull()) {
        QImage bgImg = projectModel->applyBackground(currentImg);
        QPixmap pixmap = QPixmap::fromImage(bgImg).scaled(
            ui->miniCanvasWidget->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
            );
        ui->miniCanvasWidget->setPixmap(pixmap);
    }
}

void MainWindow::updateRecentProjectsUI() {
    ui->list_RecentProjects->clear();

    QStringList recentFiles = SettingsManager::getRecentProjects();

    for (const QString &filePath : recentFiles) {
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) continue;

        QListWidgetItem *item = new QListWidgetItem(ui->list_RecentProjects);
        item->setSizeHint(QSize(400, 55));

        QWidget *rowWidget = new QWidget();
        rowWidget->setCursor(Qt::PointingHandCursor);

        QVBoxLayout *layout = new QVBoxLayout(rowWidget);
        layout->setContentsMargins(10, 5, 10, 5);
        layout->setSpacing(2);

        QLabel *nameLabel = new QLabel(fileInfo.fileName());
        nameLabel->setStyleSheet("color: #64B5F6; font-weight: bold; font-size: 14px; background: transparent;");

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

void MainWindow::updateUiStates() {
    if (!projectModel || ui->stackedWidget->currentIndex() == 0) return;

    bool hasLayers = (projectModel->getLayerCount() > 0);
    bool isPlaying = projectModel->isPlaying();
    bool canDraw = hasLayers && !isPlaying;

    ui->toolsContents->setEnabled(canDraw);
    ui->act_Cut->setEnabled(canDraw);
    ui->act_Copy->setEnabled(canDraw);
    ui->act_Paste->setEnabled(canDraw);
    ui->act_Clear->setEnabled(canDraw);
    ui->btn_DeleteLayer->setEnabled(hasLayers);
    ui->btn_DeleteFrame->setEnabled(projectModel->getFrameCount() > 1);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease ||
        (event->type() == QEvent::Resize && watched == ui->toolsScroll->viewport())) {
        QTimer::singleShot(0, this, &MainWindow::snapToolsWidth);
    }

    if (event->type() == QEvent::Resize && watched == ui->miniCanvasWidget) {
        refreshMiniMap();
    }

    if (event->type() == QEvent::Resize) {
        if (watched == ui->dockFrames)      updateFramesOrientation();
        else if (watched == ui->dockLayers) updateLayersOrientation();
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        QTabBar *bar = qobject_cast<QTabBar*>(watched);
        if (bar && bar->parentWidget() == this) {
            const int idx = bar->tabAt(static_cast<QMouseEvent*>(event)->pos());
            if (idx >= 0) {
                for (QDockWidget *d : m_docks) {
                    if (d->windowTitle() == bar->tabText(idx)) {
                        d->setFloating(true);
                        return true;
                    }
                }
            }
        }
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
    if (ui->canvasWidget->hasFloatingImage()) {
        ui->canvasWidget->clearSelectionContent();
        return;
    }

    projectModel->undo();
    ui->canvasWidget->resetLastPoint();
    ui->canvasWidget->update();
}
void MainWindow::redo() {
    projectModel->redo();
    ui->canvasWidget->update();
}

void MainWindow::clear() {
    if (ui->canvasWidget->hasActiveSelection()) {
        ui->canvasWidget->clearSelectionContent();
    } else {
        projectModel->clearCanvas();
        ui->canvasWidget->resetToolState();
    }
}

void MainWindow::selectAll() {
    QImage currentLayerImage = projectModel->getCurrentLayerImage();

    if (currentLayerImage.isNull()) {
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setImage(currentLayerImage);
}

void MainWindow::on_spin_brushSize_valueChanged(int value) {
    ui->canvasWidget->setBrushSize(value);
    ui->slider_BrushSize->setToolTip(tr("Brush Size: %1").arg(value));
}

void MainWindow::chooseAndSetColor() {
    const QColor selectedColor = QColorDialog::getColor(ui->canvasWidget->getMonoDisplayColor(), this, tr("Choose OLED Color"));

    if (selectedColor.isValid()) {
        ui->canvasWidget->setMonoDisplayColor(selectedColor);
        projectModel->setMonoColor(selectedColor);
        SettingsManager::setMonoColor(selectedColor);
        ui->canvasWidget->setPrimaryColor(selectedColor);
        currentPrimaryColor = selectedColor;
        updateColorIndicators();
    }
}

// Group D: Navigation & Dialogs
// ====================================
void MainWindow::openExportMenu() {
    ExportDialog dialog(projectModel, fileController->getCurrentProjectName(), this);
    dialog.exec();
}
