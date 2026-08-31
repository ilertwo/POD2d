#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>
#include <QKeySequenceEdit>
#include <QTableWidgetItem>
#include <QColorDialog>

struct ShortcutItem {
    QString displayName;
    QString settingsKey;
    QString defaultShortcut;
};

// List of all binds
const QList<ShortcutItem> HOTKEYS = {
    {"Pen Tool", "shortcuts/pen", "P"},
    {"Fill Tool", "shortcuts/fill", "F"},
    {"Line Tool", "shortcuts/line", "L"},
    {"Undo", "shortcuts/rectangle", "R"},
    {"Redo", "shortcuts/circle", "C"},
    {"Text Tool", "shortcuts/text", "T"},
    {"Dithering Tool", "shortcuts/dithering", "D"},
    {"Broken Line Tool", "shortcuts/brokenLine", "B"},
    {"Pan Tool", "shortcuts/pan", "Space"},
    {"Select", "shortcuts/select", "S"},
    {"Clear", "shortcuts/clear", "Delete"},
    {"Add Frame", "shortcuts/addFrame", "Ctrl+N"},
    {"Delete Frame", "shortcuts/deleteFrame", "Ctrl+Shift+D"},
    {"Add Layer", "shortcuts/addLayer", "Ctrl+Shift+N"},
    {"Delete Layer", "shortcuts/deleteLayer", "Ctrl+Alt+D"},
    {"Play Animation", "shortcuts/play", "Return"}

    // TODO:*********************************************************
};

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Settings");

    connect(ui->btn_OK, &QPushButton::clicked, this, [this]() {
        saveSettings();
        accept();
    });
    connect(ui->btn_Apply, &QPushButton::clicked, this, [this]() {
        saveSettings();
        emit settingsApplied();
    });
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->slider_Scale, &QSlider::valueChanged, this, &SettingsDialog::updateScaleLabel);
    connect(ui->btn_GridColor, &QPushButton::clicked, this, &SettingsDialog::chooseGridColor);

    loadSettings();
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
    bool showGridEnabled = settings.value("canvas/showGrid", false).toBool();

    ui->spin_DefaultWidth->setValue(settings.value("editor/defaultWidth", 128).toInt());
    ui->spin_DefaultHeight->setValue(settings.value("editor/defaultHeight", 64).toInt());
    ui->spin_UndoLimit->setValue(settings.value("editor/undoLimit", 50).toInt());
    ui->chk_AutoSave->setChecked(autoSaveEnabled);
    ui->spin_AutoSaveInterval->setValue(settings.value("editor/autoSaveInterval", 5).toInt());

    ui->cmb_Language->setCurrentIndex(settings.value("export/defaultFormat", 0).toInt());
    ui->input_VariablePrefix->setText(settings.value("export/variablePrefix", "bitmap_").toString());
    ui->chk_Progmem->setChecked(useProgmemEnabled);
    ui->chk_AutoCopy->setChecked(autoCopyEnabled);

    ui->table_Controls->setRowCount(HOTKEYS.size());
    ui->table_Controls->setColumnCount(2);

    ui->cmb_Theme->setCurrentText(settings.value("ui/theme", "dark").toString());

    ui->slider_Scale->setValue(settings.value("ui/scale", 100).toInt());
    ui->chk_ShowGrid->setChecked(showGridEnabled);
    ui->cmb_BgStyle->setCurrentText(settings.value("canvas/bgStyle", "Solid Black").toString());

    currentGridColor = settings.value("canvas/gridColor", "#333333").toString();
    setButtonColor(currentGridColor);

    ui->table_Controls->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->table_Controls->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    for (int i = 0; i < HOTKEYS.size(); ++i) {
        const auto& itemData = HOTKEYS[i];

        QTableWidgetItem *nameItem = new QTableWidgetItem(itemData.displayName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->table_Controls->setItem(i, 0, nameItem);

        QKeySequenceEdit *keyEdit = new QKeySequenceEdit(this);

        QString currentKey = settings.value(itemData.settingsKey, itemData.defaultShortcut).toString();
        keyEdit->setKeySequence(QKeySequence(currentKey));

        keyEdit->setObjectName(itemData.settingsKey);

        ui->table_Controls->setCellWidget(i, 1, keyEdit);
    }
}

void SettingsDialog::saveSettings() {
    QSettings settings("POD2d", "EditorSettings");

    settings.setValue("editor/defaultWidth", ui->spin_DefaultWidth->value());
    settings.setValue("editor/defaultHeight", ui->spin_DefaultHeight->value());
    settings.setValue("editor/undoLimit", ui->spin_UndoLimit->value());
    settings.setValue("editor/autoSave", ui->chk_AutoSave->isChecked());
    settings.setValue("editor/autoSaveInterval", ui->spin_AutoSaveInterval->value());

    settings.setValue("export/defaultFormat", ui->cmb_Language->currentIndex());
    settings.setValue("export/variablePrefix", ui->input_VariablePrefix->text().trimmed());
    settings.setValue("export/useProgmem", ui->chk_Progmem->isChecked());
    settings.setValue("export/autoCopy", ui->chk_AutoCopy->isChecked());

    settings.setValue("ui/theme", ui->cmb_Theme->currentText()); // "dark", "light", "1bit"
    settings.setValue("ui/scale", ui->slider_Scale->value());
    settings.setValue("canvas/showGrid", ui->chk_ShowGrid->isChecked());
    settings.setValue("canvas/gridColor", currentGridColor);
    settings.setValue("canvas/bgStyle", ui->cmb_BgStyle->currentText());

    for (int i = 0; i < ui->table_Controls->rowCount(); ++i) {
        QWidget *widget = ui->table_Controls->cellWidget(i, 1);
        QKeySequenceEdit *keyEdit = qobject_cast<QKeySequenceEdit*>(widget);

        if (keyEdit) {
            QString key = keyEdit->objectName();
            QString sequence = keyEdit->keySequence().toString();

            settings.setValue(key, sequence);
        }
    }
}

void SettingsDialog::updateScaleLabel(int value) {
    ui->lbl_ScaleValue->setText(QString::number(value) + "%");
}

void SettingsDialog::chooseGridColor() {
    QColor initialColor(currentGridColor.isEmpty() ? "#333333" : currentGridColor);
    QColor newColor = QColorDialog::getColor(initialColor, this, "Select Grid Color");

    if (newColor.isValid()) {
        currentGridColor = newColor.name();
        setButtonColor(currentGridColor);
        ui->gridColorLabel->setText(currentGridColor);
    }
}

void SettingsDialog::setButtonColor(const QString &hexColor) {
    ui->btn_GridColor->setText("");
    ui->btn_GridColor->setStyleSheet(
        "QPushButton { background-color: " + hexColor + "; border: 1px solid #888888; border-radius: 4px; }"
        );
}
