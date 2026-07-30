#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "oledcanvas.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->listWidget->setFlow(QListView::LeftToRight);
    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setIconSize(QSize(128, 64));
    ui->listWidget->setSpacing(5);
    ui->listWidget->setFixedHeight(100);
    ui->listWidget->setMovement(QListView::Static);

    QListWidgetItem *firstItem = new QListWidgetItem();
    firstItem->setIcon(QIcon(QPixmap::fromImage(ui->canvasWidget->getFlattenedImage())));
    firstItem->setText("1");
    ui->listWidget->addItem(firstItem);
    ui->listWidget->setCurrentRow(0);

    connect(ui->canvasWidget, &OledCanvas::frameAdded, this, [=](const QImage &thumb, int index) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setIcon(QIcon(QPixmap::fromImage(thumb)));
        item->setText(QString::number(index + 1));
        ui->listWidget->addItem(item);
    });

    connect(ui->canvasWidget, &OledCanvas::imageChanged, this, [=](const QImage &updatedImage) {
        QListWidgetItem *currentItem = ui->listWidget->currentItem();
        if (currentItem) {
            currentItem->setIcon(QIcon(QPixmap::fromImage(updatedImage)));
        }

        ui->miniCanvasWidget->setPixmap(QPixmap::fromImage(updatedImage));
    });

    connect(ui->canvasWidget, &OledCanvas::frameChanged, this, [=](int index) {
        ui->listWidget->blockSignals(true);
        ui->listWidget->setCurrentRow(index);
        ui->listWidget->blockSignals(false);
    });

    connect(ui->listWidget, &QListWidget::currentRowChanged, this, [=](int row) {
        if (row >= 0) ui->canvasWidget->setCurrentFrame(row);
    });

    connect(ui->canvasWidget, &OledCanvas::frameDeleted, this, [=](int deletedIndex) {
        QListWidgetItem *item = ui->listWidget->takeItem(deletedIndex);
        if (item) {
            delete item;
        }
    });

    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setFlow(QListView::TopToBottom);
    ui->listWidget->setGridSize(QSize(140, 90));
    //ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    connect(ui->pushButton_28, &QPushButton::clicked, ui->canvasWidget, &OledCanvas::addFrame);
    connect(ui->btm_Play, &QPushButton::clicked, ui->canvasWidget, &OledCanvas::togglePlay);

    QString basePath = QFileInfo(__FILE__).dir().absolutePath();

    ui->pushButton_3->setIcon(QIcon(basePath + "/image/lock.png"));
    ui->pushButton_2->setIcon(QIcon(basePath + "/image/waste.png"));

    ui->btn_Undo->setIcon(QIcon(basePath + "/image/undo.png"));
    ui->btn_Redo->setIcon(QIcon(basePath + "/image/redo.png"));
    ui->btn_Paste->setIcon(QIcon(basePath + "/image/paste.png"));
    ui->btn_Copy->setIcon(QIcon(basePath + "/image/copy.png"));
    ui->btn_Cut->setIcon(QIcon(basePath + "/image/cut.png"));

    ui->btn_Pen->setIcon(QIcon(basePath + "/image/pen.png"));

    ui->btn_Dithering->setIcon(QIcon(basePath + "/image/dithering.png"));
    ui->btn_Pain->setIcon(QIcon(basePath + "/image/pain.png"));
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

    connect(ui->canvasWidget, &OledCanvas::isPlayingChanged, this, [=](bool playing) {
        ui->btm_Play->setIcon(playing ? QIcon(basePath + "/image/stop.png") : QIcon(basePath + "/image/play.png"));
    });

    setWindowTitle("POD2d");

    connect(ui->canvasWidget, &OledCanvas::imageChanged, this, [this](const QImage &img) {
        QPixmap pixmap = QPixmap::fromImage(img).scaled(
            ui->miniCanvasWidget->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
            );

        ui->miniCanvasWidget->setPixmap(pixmap);
    });


    connect(ui->listWidget_2, &QListWidget::currentRowChanged, ui->canvasWidget, &OledCanvas::setCurrentLayer);

    connect(ui->canvasWidget, &OledCanvas::layersListChanged, this, &MainWindow::rebuildLayersList);

    connect(ui->canvasWidget, &OledCanvas::activeLayerChanged, this, [=](int index) {
        ui->listWidget_2->blockSignals(true);
        if (index >= 0 && index < ui->listWidget_2->count()) {
            ui->listWidget_2->setCurrentRow(index);
        }
        ui->listWidget_2->blockSignals(false);
    });

    connect(ui->canvasWidget, &OledCanvas::layerThumbnailUpdated, this, [=](int index) {
        if (index >= 0 && index < ui->listWidget_2->count()) {
            QImage thumb = ui->canvasWidget->getLayerThumbnail(index);
            ui->listWidget_2->item(index)->setIcon(QIcon(QPixmap::fromImage(thumb)));
        }
    });

    connect(ui->canvasWidget, &OledCanvas::frameChanged, this, [=](int index) {
        rebuildLayersList();
    });

    ui->listWidget_2->setViewMode(QListView::ListMode);
    ui->listWidget_2->setIconSize(QSize(64, 32));
    ui->listWidget_2->setSpacing(3);

    rebuildLayersList();

    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::createProject);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::buttonProjects);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::buttonExemples);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::saveProject);
    connect(ui->btn_Clear, &QPushButton::clicked, this, &MainWindow::clear);
    connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::closeFrameCreateProject);
    connect(ui->pushButton_8, &QPushButton::clicked, this, &MainWindow::buttonCreate);
    connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::printCode);
    connect(ui->btn_Undo, &QPushButton::clicked, this, &MainWindow::undo);
    connect(ui->btn_Redo, &QPushButton::clicked, this, &MainWindow::redo);
    connect(ui->pushButton_12, &QPushButton::clicked, this, &MainWindow::addLayer);
    connect(ui->btn_SetColor, &QPushButton::clicked, this, &MainWindow::on_chooseColorButton_clicked);

    connect(ui->btn_Pen, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Brush); });
    connect(ui->spinBox_2, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::on_spin_brushSize_valueChanged);

    connect(ui->btn_Dithering, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Dithering); });
    connect(ui->btn_Line, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Line); });
    connect(ui->btn_Rectangle, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Rectangle); });
    connect(ui->btn_Circle, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Circle); });
    connect(ui->btn_Pain, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Fill); });
    connect(ui->btn_BrokenLine, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::BrokenLine); });
    connect(ui->btn_Text, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Text); });
    connect(ui->btn_Pan, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Pan); });
    connect(ui->btn_Rotate, &QPushButton::clicked, this, [this](){ ui->canvasWidget->rotateFloatingImage(); });

    connect(ui->btn_Copy, &QPushButton::clicked, this, [this](){ ui->canvasWidget->copyLayer(); });
    connect(ui->btn_Cut, &QPushButton::clicked, this, [this](){ ui->canvasWidget->cutLayer(); });
    connect(ui->btn_Paste, &QPushButton::clicked, this, [this](){ ui->canvasWidget->pasteToLayer(); });
    connect(ui->btn_Select, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Select); });
    connect(ui->pushButton_30, &QPushButton::clicked, ui->canvasWidget, &OledCanvas::deleteCurrentFrame);
    connect(ui->pushButton_31, &QPushButton::clicked, ui->canvasWidget, &OledCanvas::deleteCurrentLayer);
}

//деструктор
MainWindow::~MainWindow()
{
    delete ui;
    delete scene;
}

void MainWindow::createProject(){
    openFrameCreateProject();
    //...
}



void MainWindow::saveProject()
{
    QString path = QFileDialog::getSaveFileName(
        this,
        "Save project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "Pod2D Project (*.pod2d)"
        );

    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    file.write(ui->canvasWidget->saveProjectData());
    file.close();

    QPixmap pixmap("./image/save.png");
    ui->IsSaveLabel->setPixmap(pixmap);
}

void MainWindow::openProject()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        "Open project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "Pod2D Project (*.pod2d)"
        );

    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    if (ui->canvasWidget->loadProjectData(data)) {

        int maxLayer = ui->canvasWidget->getLayerCount() - 1;

        ui->listWidget->clear();

        for (int i = 0; i < ui->canvasWidget->getFrameCount(); ++i) {
            QImage thumb = ui->canvasWidget->getFrameThumbnail(i);
            QListWidgetItem *item = new QListWidgetItem();
            item->setIcon(QIcon(QPixmap::fromImage(thumb)));
            item->setText(QString::number(i + 1));
            ui->listWidget->addItem(item);
        }

        ui->listWidget->blockSignals(true);
        ui->listWidget->setCurrentRow(ui->canvasWidget->getCurrentFrameIndex());
        ui->listWidget->blockSignals(false);

        ui->stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::buttonProjects(){}
void MainWindow::buttonExemples(){}
void MainWindow::recentProject(){}
void MainWindow::allLayer(){}


void MainWindow::setCurrentLayer(){
    //ui->canvasWidget->setCurrentLayer(index);
}

void MainWindow::addLayer() {
    ui->canvasWidget->addLayer();

    int newMax = ui->canvasWidget->getLayerCount() - 1;

}

void MainWindow::deleteCurrentLayer() {
    ui->canvasWidget->deleteCurrentLayer();

    int newMax = ui->canvasWidget->getLayerCount() - 1;
}


void MainWindow::setScale(int newScale)
{
    if (newScale < 1) return;

    ui->canvasWidget->update();
}

void MainWindow::undo()
{
    ui->canvasWidget->undo();
}

void MainWindow::redo()
{
    ui->canvasWidget->redo();
}

void MainWindow::on_chooseColorButton_clicked()
{
    QColor selectedColor = QColorDialog::getColor(Qt::white, this, "Обери колір");

    if (selectedColor.isValid()) {
        //
    }
}

void MainWindow::on_spin_brushSize_valueChanged(int value) {
    ui->canvasWidget->setBrushSize(value);
}

void MainWindow::buttonCreate(){
    ui->stackedWidget->setCurrentIndex(1);
    closeFrameCreateProject();
}
void MainWindow::buttonCancel(){
    closeFrameCreateProject();
}

void MainWindow::clear() {
    ui->canvasWidget->clearCanvas();
}

void MainWindow::printCode() {
    bool isOptimize = ui->checkBox->checkState();
    bool language = ui->checkBox_2->checkState();
    bool exportAnimation = ui->checkBox_3->isChecked();

    QString finalCode = ui->canvasWidget->generateExportCode(isOptimize, language, exportAnimation);
    ui->plainTextEdit->setPlainText(finalCode);
}

void MainWindow::openFrameCreateProject()
{
    if (windowCreateProject != nullptr) {
        if (windowCreateProject->isVisible())
            return;
    }

    windowCreateProject = new QWidget();
    windowCreateProject->setWindowTitle("Create project");
    windowCreateProject->resize(ui->frame->width() + 20, ui->frame->height() + 20);

    QWidget* originalParent = ui->frame->parentWidget();
    ui->frame->setParent(windowCreateProject);

    QVBoxLayout* layout = new QVBoxLayout(windowCreateProject);
    layout->addWidget(ui->frame);

    connect(ui->pushButton_5, &QPushButton::clicked, windowCreateProject, &QWidget::close);

    connect(windowCreateProject, &QWidget::destroyed, this, [=]() {
        ui->frame->setParent(originalParent);
        originalParent->layout()->addWidget(ui->frame);
        windowCreateProject = nullptr;
    });

    windowCreateProject->show();
}

void MainWindow::closeFrameCreateProject()
{
    if (windowCreateProject != nullptr)
        windowCreateProject->close();
}

void MainWindow::rebuildLayersList() {
    ui->listWidget_2->blockSignals(true);
    ui->listWidget_2->clear();

    int count = ui->canvasWidget->getLayerCount();
    for (int i = 0; i < count; ++i) {
        QImage thumb = ui->canvasWidget->getLayerThumbnail(i);
        QListWidgetItem *item = new QListWidgetItem();
        item->setIcon(QIcon(QPixmap::fromImage(thumb)));

        if (i == 0) item->setText("0");
        else item->setText("" + QString::number(i));

        ui->listWidget_2->addItem(item);
    }

    ui->listWidget_2->setCurrentRow(ui->canvasWidget->getCurrentLayerIndex());
    ui->listWidget_2->blockSignals(false);
}
