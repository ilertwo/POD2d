#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "pixelcanvas.h"
#include "codegenerator.h"
#include "projectmodel.h"
#include "createprojectdialog.h"
#include "exportdialog.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QColorDialog>
#include <QFileInfo>
#include <QDir>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTimer>

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

    this->showMaximized();
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
    ui->btn_MiniCanvasWidget->setIcon(QIcon(basePath + "/image/miniCanvas.png"));
    ui->btn_FrameList->setIcon(QIcon(basePath + "/image/frame.png"));
    ui->btn_LayerList->setIcon(QIcon(basePath + "/image/layer.png"));
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
}

void MainWindow::connectModelToLists() {
    connectFramesList();
    connectLayersList();
    connectMiniCanvas();

    connect(projectModel, &ProjectModel::imageChanged, this, [this]() {
        ui->canvasWidget->update();
    });
}

void MainWindow::connectFramesList() {
    QListWidget* framesList = ui->framesListWidget;

    /*connect(projectModel, &ProjectModel::frameAdded, this, [framesList](const QImage &thumb, int index) {
        QListWidgetItem *item = new QListWidgetItem(QString::number(index + 1));
        item->setIcon(QIcon(QPixmap::fromImage(thumb)));
        framesList->addItem(item);
    });

    connect(projectModel, &ProjectModel::frameDeleted, this, [framesList](int deletedIndex) {
        delete framesList->takeItem(deletedIndex);
    });*/

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
        if (QListWidgetItem *frameItem = ui->framesListWidget->currentItem()) {
            frameItem->setIcon(QIcon(QPixmap::fromImage(flatImg)));
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

    connect(projectModel, &ProjectModel::imageChanged, this, [this](const QImage &/*flatImg*/) {
        if (QListWidgetItem *layerItem = ui->layersListWidget->currentItem()) {
            int activeIdx = projectModel->getCurrentLayerIndex();
            QImage layerThumb = projectModel->getLayerThumbnail(activeIdx);

            layerItem->setIcon(QIcon(QPixmap::fromImage(layerThumb)));
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
    connect(ui->btn_SetColor, &QPushButton::clicked, this, &MainWindow::chooseAndSetColor);

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

// ==========================================
// 3. Logic methods and handlers
// ==========================================

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
        rebuildFramesList();
        ui->stackedWidget->setCurrentIndex(1);

        QTimer::singleShot(50, this, [this]() {
            ui->canvasWidget->fitToScreen();
        });
    } else {
        QMessageBox::warning(this, "Error", "The project file is corrupted or has an invalid format!");
    }
}

void MainWindow::rebuildFramesList() {
    QListWidget* framesList = ui->framesListWidget;

    const QSignalBlocker blocker(framesList);
    framesList->clear();

    const int frameCount = projectModel->getFrameCount();

    for (int i = 0; i < frameCount; ++i) {
        QListWidgetItem *item = new QListWidgetItem(QString::number(i + 1));

        QImage thumb = projectModel->getFrameThumbnail(i);
        item->setIcon(QIcon(QPixmap::fromImage(thumb)));

        framesList->addItem(item);
    }

    framesList->setCurrentRow(projectModel->getCurrentFrameIndex());
}

void MainWindow::addLayer() {
    projectModel->addLayer();
}

void MainWindow::setScale(int newScale) {
    if (newScale < 1) return;

    ui->canvasWidget->update();
}

void MainWindow::deleteCurrentLayer() { projectModel->deleteCurrentLayer(); }

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

void MainWindow::on_spin_brushSize_valueChanged(int value) { ui->canvasWidget->setBrushSize(value); }

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

void MainWindow::chooseAndSetColor() {
    const QColor selectedColor = QColorDialog::getColor(Qt::white, this, "Choose a color");

    if (selectedColor.isValid()) {
        // ui->canvasWidget->setCurrentColor(selectedColor);
    }
}

void MainWindow::createProject() {
    CreateProjectDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        ui->stackedWidget->setCurrentIndex(1);

        QTimer::singleShot(50, this, [this]() {
            ui->canvasWidget->fitToScreen();
        });
    }
}

void MainWindow::openExportMenu() {
    ExportDialog dialog(projectModel, this);

    dialog.exec();
}
