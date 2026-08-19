#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Settings");
    loadSettings();

    connect(ui->btn_OK, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->btn_OK, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btn_Apply,  &QPushButton::clicked, this, &SettingsDialog::saveSettings);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setActiveTab(int index) {
    if (index >= 0 && index < ui->tabWidget->count()) {
        ui->tabWidget->setCurrentIndex(index);
    }
}

void SettingsDialog::loadSettings() {
    QSettings settings("POD2d", "EditorSettings");

    bool autoSaveEnabled = settings.value("editor/autoSave", false).toBool();
    bool useProgmemEnabled = settings.value("export/useProgmem", true).toBool();
    bool autoCopyEnabled = settings.value("export/autoCopy", false).toBool();

    ui->spin_DefaultWidth->setValue(settings.value("editor/defaultWidth", 128).toInt());
    ui->spin_DefaultHeight->setValue(settings.value("editor/defaultHeight", 64).toInt());
    ui->spin_UndoLimit->setValue(settings.value("editor/undoLimit", 50).toInt());
    ui->chk_AutoSave->setChecked(autoSaveEnabled);
    ui->spin_AutoSaveInterval->setValue(settings.value("editor/autoSaveInterval", 5).toInt());

    ui->cmb_Language->setCurrentIndex(settings.value("export/defaultFormat", 0).toInt());
    ui->input_VariablePrefix->setPlainText(settings.value("export/variablePrefix", "bitmap_").toString());
    ui->chk_Progmem->setChecked(useProgmemEnabled);
    ui->chk_AutoCopy->setChecked(autoCopyEnabled);
}

void SettingsDialog::saveSettings() {
    QSettings settings("POD2d", "EditorSettings");

    settings.setValue("editor/defaultWidth", ui->spin_DefaultWidth->value());
    settings.setValue("editor/defaultHeight", ui->spin_DefaultHeight->value());
    settings.setValue("editor/undoLimit", ui->spin_UndoLimit->value());
    settings.setValue("editor/autoSave", ui->chk_AutoSave->isChecked());
    settings.setValue("editor/autoSaveInterval", ui->spin_AutoSaveInterval->value());

    settings.setValue("export/defaultFormat", ui->cmb_Language->currentIndex());
    settings.setValue("export/variablePrefix", ui->input_VariablePrefix->toPlainText().trimmed());
    settings.setValue("export/useProgmem", ui->chk_Progmem->isChecked());
    settings.setValue("export/autoCopy", ui->chk_AutoCopy->isChecked());

    QDialog::accept();
}
