#include "createprojectdialog.h"
#include "ui_createprojectdialog.h"

#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>

CreateProjectDialog::CreateProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateProjectDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Create project");

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    ui->input_Location->setPlaceholderText(defaultPath);

    QSettings settings("POD2d", "EditorSettings");
    int defaultWidth = settings.value("editor/defaultWidth", 128).toInt();
    int defaultHeight = settings.value("editor/defaultHeight", 64).toInt();

    ui->spin_Width->setValue(defaultWidth);
    ui->spin_Height->setValue(defaultHeight);

    QString basePath = QFileInfo(__FILE__).dir().absolutePath();
    ui->btn_Browse->setIcon(QIcon(basePath + "/image/save.png"));

    connect(ui->btn_ConfirmCreate, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->btn_Browse,  &QPushButton::clicked, this, &CreateProjectDialog::on_btn_Browse_clicked);
}

CreateProjectDialog::~CreateProjectDialog()
{
    delete ui;
}

int CreateProjectDialog::getWidth() const {
    return ui->spin_Width->value();
}

int CreateProjectDialog::getHeight() const {
    return ui->spin_Height->value();
}

QString CreateProjectDialog::getProjectName() const {
    return ui->input_ProjectName->text().trimmed();
}

void CreateProjectDialog::on_btn_Browse_clicked() {
    QString currentPath = ui->input_Location->text().trimmed();

    if (currentPath.isEmpty() || !QDir(currentPath).exists()) {
        currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString dir = QFileDialog::getExistingDirectory(this,
                                                    "Select a folder to save to.",
                                                    currentPath,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);

    if (!dir.isEmpty()) {
        ui->input_Location->setText(dir);
    }
}

QString CreateProjectDialog::getFullFilePath() const {
    QString currentPath = ui->input_Location->text().trimmed();
    if (currentPath.isEmpty()) {
        currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString name = getProjectName();
    if (name.isEmpty()) {
        name = "Untitled";
    }

    return QDir(currentPath).filePath(name + ".pod2d");
}

bool CreateProjectDialog::isRGBMode() const {
    return ui->cmb_ColorMode->currentIndex() == 1;
}
