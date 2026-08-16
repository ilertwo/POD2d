#include "exportdialog.h"
#include "ui_exportdialog.h"
#include "projectmodel.h"
#include "codegenerator.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

ExportDialog::ExportDialog(ProjectModel *model, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog),
    projectModel(model)
{
    ui->setupUi(this);


    this->setWindowTitle("Export");
    Q_ASSERT_X(projectModel != nullptr, "ExportDialog", "Critical error: ProjectModel was not passed!");

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
        exportAnimation
        );

    ui->codeOutputTextEdit->setPlainText(finalCode);
}

void ExportDialog::copyToClipboard(){}

void ExportDialog::saveProjectFile() {
    const QString path = QFileDialog::getSaveFileName(
        this, "Save project",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
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
}

void ExportDialog::exportToPng(){}
void ExportDialog::exportToGif(){}
