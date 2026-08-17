#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Settings");
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
