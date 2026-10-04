#include "createprojectdialog.h"
#include "ui_createprojectdialog.h"
#include "settingsmanager.h"

#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>

CreateProjectDialog::CreateProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateProjectDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Create project");

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    ui->input_Location->setPlaceholderText(defaultPath);

    int defaultWidth = SettingsManager::getValue("editor/defaultWidth", 128).toInt();
    int defaultHeight = SettingsManager::getValue("editor/defaultHeight", 64).toInt();
    int defaultColorMode = SettingsManager::getValue("editor/defaultColorMode", 0).toInt();

    ui->spin_Width->setValue(defaultWidth);
    ui->spin_Height->setValue(defaultHeight);
    ui->cmb_ColorMode->setCurrentIndex(defaultColorMode);

    QString basePath = QFileInfo(__FILE__).dir().absolutePath();

    connect(ui->btn_ConfirmCreate, &QPushButton::clicked, this, [this]() {
        SettingsManager::setValue("editor/defaultColorMode", ui->cmb_ColorMode->currentIndex());
        this->accept();
    });

    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->btn_Browse,  &QPushButton::clicked, this, &CreateProjectDialog::on_btn_Browse_clicked);

    setTheme();
}

void CreateProjectDialog::setTheme() {
    QString basePath = QFileInfo(__FILE__).dir().absolutePath();
    QString theme = SettingsManager::getTheme();

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

    setBtnIcon(ui->btn_Browse, "B", "path.png");
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
                                                    QFileDialog::ShowDirsOnly);

    if (!dir.isEmpty()) {
        ui->input_Location->setText(dir);
    }
}

QString CreateProjectDialog::getFullFilePath() const {
    QString currentPath = ui->input_Location->text().trimmed();
    if (currentPath.isEmpty()) {
        currentPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString baseName = getProjectName();
    bool isDefaultName = false;

    if (baseName.isEmpty() || baseName == "Untitled") {
        baseName = "Untitled";
        isDefaultName = true;
    }

    QDir dir(currentPath);
    QString finalName = baseName;
    QString fullPath = dir.filePath(finalName + ".pod2d");

    if (isDefaultName) {
        int counter = 1;
        while (QFile::exists(fullPath)) {
            finalName = QString("%1_%2").arg(baseName).arg(counter);
            fullPath = dir.filePath(finalName + ".pod2d");
            counter++;
        }
    }
    else if (QFile::exists(fullPath)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning((QWidget*)this->parent(), "File Exists",
                                     QString("A project named '%1' already exists in this folder.\nDo you want to overwrite it?").arg(baseName),
                                     QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return QString();
        }
    }

    ui->input_ProjectName->setText(finalName);

    return fullPath;
}

bool CreateProjectDialog::isRGBMode() const {
    return ui->cmb_ColorMode->currentIndex() == 1;
}
