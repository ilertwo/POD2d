#include "exportdialog.h"
#include "ui_exportdialog.h"
#include "projectmodel.h"
#include "codegenerator.h"

#include "gif.h"

#include <QPainter>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSettings>
#include <QClipboard>

ExportDialog::ExportDialog(ProjectModel *model, const QString &projName, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog),
    projectModel(model),
    projectName(projName)
{
    ui->setupUi(this);

    QSettings settings("POD2d", "EditorSettings");
    int defaultFormat = settings.value("export/defaultFormat", 0).toInt();
    ui->cmb_Language->setCurrentIndex(defaultFormat);

    this->setWindowTitle("Export");
    Q_ASSERT_X(projectModel != nullptr, "ExportDialog", "Critical error: ProjectModel was not passed!");

    if (projectModel->getIsRGB()) {
        ui->chk_Optimize->setEnabled(false);
        ui->chk_Optimize->setToolTip("Optimization is currently supported only for Monochrome projects.");
    }

    // CODE GENERATION
    connect(ui->btn_Generate, &QPushButton::clicked, this, &ExportDialog::generateCode);
    connect(ui->btn_CopyCode, &QPushButton::clicked, this, &ExportDialog::copyToClipboard);

    // EXPORT TO FILE
    connect(ui->btn_SaveProject, &QPushButton::clicked, this, &ExportDialog::saveProjectFile);
    connect(ui->btn_ExportPng, &QPushButton::clicked, this, &ExportDialog::exportToPng);
    connect(ui->btn_ExportGif, &QPushButton::clicked, this, &ExportDialog::exportToGif);

    // CLOSE
    connect(ui->btn_Close, &QPushButton::clicked, this, &QDialog::reject);
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::generateCode() {
    bool isOptimize = ui->chk_Optimize->isChecked();
    bool language = (ui->cmb_Language->currentIndex() == 0);
    bool exportAnimation = ui->chk_Animation->isChecked();

    bool isRGBMode = projectModel->getIsRGB();

    const int frameCount = projectModel->getFrameCount();
    QList<QImage> flattenedFrames;
    flattenedFrames.reserve(frameCount);

    for (int i = 0; i < frameCount; ++i) {
        flattenedFrames.append(projectModel->getFlattenedFrame(i));
    }

    int currentIndex = projectModel->getCurrentFrameIndex();

    QString finalCode = CodeGenerator::generateExportCode(
        flattenedFrames,
        currentIndex,
        isOptimize,
        language,
        exportAnimation,
        isRGBMode
        );

    ui->codeOutputTextEdit->setPlainText(finalCode);

    QSettings settings("POD2d", "EditorSettings");
    if (settings.value("export/autoCopy", false).toBool()) {
        QGuiApplication::clipboard()->setText(finalCode);
    }
}

void ExportDialog::copyToClipboard(){
    QString finalCode = ui->codeOutputTextEdit->toPlainText();
    QGuiApplication::clipboard()->setText(finalCode);
}

void ExportDialog::saveProjectFile() {
    QString defaultName = projectName.isEmpty() ? "project.pod2d" : projectName + ".pod2d";

    const QString path = QFileDialog::getSaveFileName(
        this, "Save project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultName,
        "Pod2D Project (*.pod2d)"
        );

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Failed to save the file!");
        return;
    }

    file.write(projectModel->saveProjectData());
    file.close();

    //ui->IsSaveLabel->setPixmap(QPixmap(":/image/save.png"));
    QMessageBox::information(this, "Success", "Project exported successfully!");
}

void ExportDialog::exportToPng() {
    QString defaultName = projectName.isEmpty() ? "frame.png" : projectName + ".png";

    QString path = QFileDialog::getSaveFileName(
        this, "Export to PNG",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultName,
        "PNG Image (*.png)"
        );

    if (path.isEmpty()) return;

    int currentIndex = projectModel->getCurrentFrameIndex();

    QImage frame = projectModel->getFlattenedFrame(currentIndex);

    if (frame.save(path)) {
        QMessageBox::information(this, "Success", "Frame exported successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save frame.");
    }
}

void ExportDialog::exportToGif() {

    QString defaultName = projectName.isEmpty() ? "animation.gif" : projectName + ".gif";
    QString path = QFileDialog::getSaveFileName(
        this, "Export to GIF",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultName,
        "GIF Animation (*.gif)"
        );

    if (path.isEmpty()) return;

    int frameCount = projectModel->getFrameCount();
    if (frameCount == 0) return;

    QImage firstFrame = projectModel->getFlattenedFrame(0);
    int width = firstFrame.width();
    int height = firstFrame.height();

    int delay = 10;

    GifWriter g;
    GifBegin(&g, path.toLocal8Bit().data(), width, height, delay);

    for (int i = 0; i < frameCount; ++i) {
        QImage img = projectModel->getFlattenedFrame(i).convertToFormat(QImage::Format_RGBA8888);

        GifWriteFrame(&g, img.constBits(), width, height, delay);
    }

    GifEnd(&g);

    QMessageBox::information(this, "Success", "Animation exported to GIF successfully!");
}
