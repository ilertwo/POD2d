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

    connect(ui->canvasWidget, &OledCanvas::isPlayingChanged, this, [=](bool playing) {
        ui->pushButton_29->setText(playing ? "Stop" : "Play");
    });

    connect(ui->canvasWidget, &OledCanvas::frameDeleted, this, [=](int deletedIndex) {
        QListWidgetItem *item = ui->listWidget->takeItem(deletedIndex);
        if (item) {
            delete item;
        }
    });

    connect(ui->pushButton_28, &QPushButton::clicked, ui->canvasWidget, &OledCanvas::addFrame);
    connect(ui->pushButton_29, &QPushButton::clicked, ui->canvasWidget, &OledCanvas::togglePlay);

    QString basePath = QFileInfo(__FILE__).dir().absolutePath();

    ui->pushButton_3->setIcon(QIcon(basePath + "/image/lock.png"));
    ui->pushButton_2->setIcon(QIcon(basePath + "/image/waste.png"));

    setWindowTitle("POD2d");

    connect(ui->canvasWidget, &OledCanvas::imageChanged, this, [this](const QImage &img) {
        QPixmap pixmap = QPixmap::fromImage(img).scaled(
            ui->miniCanvasWidget->size(),
            Qt::KeepAspectRatio,
            Qt::FastTransformation
            );

        ui->miniCanvasWidget->setPixmap(pixmap);
    });

    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::createProject);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::buttonProjects);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::buttonExemples);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::saveProject);
    connect(ui->pushButton_6, &QPushButton::clicked, this, &MainWindow::clear);
    connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::closeFrameCreateProject);
    connect(ui->pushButton_8, &QPushButton::clicked, this, &MainWindow::buttonCreate);
    connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::printCode);
    connect(ui->pushButton_10, &QPushButton::clicked, this, &MainWindow::undo);
    connect(ui->pushButton_11, &QPushButton::clicked, this, &MainWindow::redo);
    connect(ui->pushButton_12, &QPushButton::clicked, this, &MainWindow::addLayer);
    connect(ui->pushButton_13, &QPushButton::clicked, this, &MainWindow::setCurrentLayer);
    connect(ui->pushButton_14, &QPushButton::clicked, this, &MainWindow::on_chooseColorButton_clicked);

    connect(ui->pushButton_24, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Pen); });
    connect(ui->pushButton_18, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Line); });
    connect(ui->pushButton_20, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Rectangle); });
    connect(ui->pushButton_19, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Circle); });
    connect(ui->pushButton_21, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Fill); });
    connect(ui->pushButton_22, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::BrokenLine); });
    connect(ui->pushButton_23, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Text); });
    connect(ui->pushButton_25, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Pan); });
    connect(ui->pushButton_26, &QPushButton::clicked, this, [this](){ ui->canvasWidget->rotateFloatingImage(); });

    connect(ui->pushButton_17, &QPushButton::clicked, this, [this](){ ui->canvasWidget->copyLayer(); });
    connect(ui->pushButton_15, &QPushButton::clicked, this, [this](){ ui->canvasWidget->cutLayer(); });
    connect(ui->pushButton_16, &QPushButton::clicked, this, [this](){ ui->canvasWidget->pasteToLayer(); });
    connect(ui->pushButton_27, &QPushButton::clicked, this, [this](){ ui->canvasWidget->setTool(DrawTool::Select); });
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
        ui->spinBox->setMaximum(maxLayer);
        ui->spinBox->setValue(maxLayer);

        ui->stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::buttonProjects(){}
void MainWindow::buttonExemples(){}
void MainWindow::recentProject(){}
void MainWindow::allLayer(){}


void MainWindow::setCurrentLayer(){
    int index = ui->spinBox->value();
    ui->canvasWidget->setCurrentLayer(index);
}

void MainWindow::addLayer() {
    ui->canvasWidget->addLayer();

    int newMax = ui->canvasWidget->getLayerCount() - 1;

    ui->spinBox->setMaximum(newMax);

    ui->spinBox->setValue(newMax);
}

void MainWindow::deleteCurrentLayer() {
    ui->canvasWidget->deleteCurrentLayer();

    int newMax = ui->canvasWidget->getLayerCount() - 1;

    if (newMax < 0) newMax = 0;

    ui->spinBox->setMaximum(newMax);

    ui->spinBox->setValue(ui->canvasWidget->getCurrentLayerIndex());
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
    QString arduinoCode = ui->canvasWidget->generateExportCode(isOptimize, language);

    ui->plainTextEdit->setPlainText(arduinoCode);
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
