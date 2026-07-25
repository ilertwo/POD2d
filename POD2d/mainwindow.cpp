#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "oledcanvas.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString basePath = QFileInfo(__FILE__).dir().absolutePath();

    ui->pushButton_3->setIcon(QIcon(basePath + "/image/lock.png"));
    ui->pushButton_2->setIcon(QIcon(basePath + "/image/waste.png"));

    setWindowTitle("Canvas");

    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::createProject);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::buttonProjects);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::buttonExemples);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::saveInTxt);
    connect(ui->pushButton_6, &QPushButton::clicked, this, &MainWindow::clear);
    connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::closeFrameCreateProject);
    connect(ui->pushButton_8, &QPushButton::clicked, this, &MainWindow::buttonCreate);
    connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::printCode);
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



void MainWindow::openProject()
{

    QString path = QFileDialog::getOpenFileName(
        this,
        "Open project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "Text Files (*.txt)"
        );

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();

    file.close();
    ui->canvasWidget->generateImage(content);
    printCode();
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::saveInTxt()
{
    QString path = QFileDialog::getSaveFileName(
        this,
        "Save project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "Text Files (*.txt)"
        );

    if (path.isEmpty()) {
        return;
    }

    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);

    out << ui->canvasWidget->generateArduinoCode();

    file.close();

    QPixmap pixmap("./image/save.png");
    ui->IsSaveLabel->setPixmap(pixmap);
}

void MainWindow::buttonProjects(){}
void MainWindow::buttonExemples(){}
void MainWindow::recentProject(){}

void MainWindow::moveLayer(){}
void MainWindow::prevLayer(){}
void MainWindow::nextLayer(){}
void MainWindow::allLayer(){}
void MainWindow::undo(){}///
void MainWindow::redo(){}///
void MainWindow::setScale(){}///
void MainWindow::showImage(){}///




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
    QString arduinoCode = ui->canvasWidget->generateArduinoCode();

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
