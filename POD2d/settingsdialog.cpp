#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settingsmanager.h"

#include <QSettings>
#include <QKeySequenceEdit>
#include <QTableWidgetItem>
#include <QColorDialog>
#include <QMessageBox>

struct ShortcutItem {
    QString displayName;
    QString settingsKey;
    QString defaultShortcut;
};

// List of all binds
const QList<ShortcutItem> HOTKEYS = {
    {"Pen Tool", "shortcuts/pen", "P"},
    {"Eraser Tool", "shortcuts/eraser", "E"},
    {"Fill Tool", "shortcuts/fill", "F"},
    {"Pipette Tool", "shortcuts/pipette", "I"},
    {"Line Tool", "shortcuts/line", "L"},
    {"Rectangle", "shortcuts/rectangle", "R"},
    {"Circle", "shortcuts/circle", "C"},
    {"Text Tool", "shortcuts/text", "T"},
    {"Dithering Tool", "shortcuts/dithering", "D"},
    {"Broken Line Tool", "shortcuts/brokenLine", "B"},
    {"Pan Tool", "shortcuts/pan", "Space"},
    {"Selection", "shortcuts/lassoSelection", "S"},
    {"Lasso Selection", "shortcuts/select", ""},
    {"Shape Selection", "shortcuts/shapeSelection", ""},
    {"Rotate", "shortcuts/rotate", ""},
    {"Lighten", "shortcuts/lighten", ""},
    {"Vertical Miror", "shortcuts/verticalMiror", ""},
    {"Center View", "shortcuts/centerView", ""},
    {"Clear", "shortcuts/clear", "Delete"},
    {"Add Frame", "shortcuts/addFrame", "Ctrl+N"},
    {"Delete Frame", "shortcuts/deleteFrame", "Ctrl+Shift+D"},
    {"Add Layer", "shortcuts/addLayer", "Ctrl+Shift+N"},
    {"Delete Layer", "shortcuts/deleteLayer", "Ctrl+Alt+D"},
    {"Play Animation", "shortcuts/play", "Return"},
    {"Mini Map", "shortcuts/miniMap", ""},
    {"Frames", "shortcuts/hideFrames", ""},
    {"Layers", "shortcuts/hideLayers", ""},
    {"Edit Mode", "shortcuts/editMode", ""},
    {"Undo", "shortcuts/undo", "Ctrl+Z"},
    {"Redo", "shortcuts/redo", "Ctrl+Y"},
    {"Copy", "shortcuts/copy", "Ctrl+C"},
    {"Cut", "shortcuts/cut", "Ctrl+X"},
    {"Paste", "shortcuts/paste", "Ctrl+V"},
    {"Export", "shortcuts/export", "Ctrl+E"}

    // TODO:*********************************************************
};

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    this->setWindowTitle(tr("Settings"));

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

    connect(ui->btn_ResetDefaults, &QPushButton::clicked, this, [this]() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, tr("Reset Settings"),
                                      tr("Are you sure you want to reset ALL settings to default?"),
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QSettings("POD2d", "EditorSettings").clear();
            loadSettings();
        }
    });
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

    ui->spin_DefaultWidth->setValue(SettingsManager::getValue("editor/defaultWidth", 128).toInt());
    ui->spin_DefaultHeight->setValue(SettingsManager::getValue("editor/defaultHeight", 64).toInt());
    ui->spin_UndoLimit->setValue(SettingsManager::getUndoLimit());
    ui->spin_MaxFrames->setValue(SettingsManager::getMaxFrames());
    ui->spin_MaxLayers->setValue(SettingsManager::getMaxLayers());
    ui->chk_AutoSave->setChecked(SettingsManager::getAutoSave());
    ui->spin_AutoSaveInterval->setValue(SettingsManager::getAutoSaveInterval());

    ui->cmb_Language->setCurrentIndex(SettingsManager::getValue("export/defaultFormat", 0).toInt());
    ui->input_VariablePrefix->setText(SettingsManager::getValue("export/variablePrefix", "bitmap_").toString());
    ui->cmb_ByteFormat->setCurrentIndex(SettingsManager::getExportByteFormat());
    ui->chk_Progmem->setChecked(SettingsManager::getValue("export/useProgmem", true).toBool());
    ui->chk_AutoCopy->setChecked(SettingsManager::getValue("export/autoCopy", false).toBool());
    ui->chk_RightClickEraser->setChecked(SettingsManager::getRightClickEraser());

    ui->cmb_Theme->setCurrentText(SettingsManager::getTheme());
    ui->slider_Scale->setValue(SettingsManager::getScale());
    ui->chk_ShowGrid->setChecked(SettingsManager::getShowGrid());
    ui->cmb_BgStyle->setCurrentText(SettingsManager::getBgStyle());

    currentGridColor = SettingsManager::getGridColor().name();
    setButtonColor(currentGridColor);

    ui->table_Controls->setRowCount(HOTKEYS.size());
    ui->table_Controls->setColumnCount(2);
    ui->table_Controls->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->table_Controls->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    for (int i = 0; i < HOTKEYS.size(); ++i) {
        const auto& itemData = HOTKEYS[i];

        QTableWidgetItem *nameItem = new QTableWidgetItem(tr(itemData.displayName.toUtf8().constData()));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->table_Controls->setItem(i, 0, nameItem);

        QKeySequenceEdit *keyEdit = new QKeySequenceEdit(this);
        QString currentKey = SettingsManager::getValue(itemData.settingsKey, itemData.defaultShortcut).toString();
        keyEdit->setKeySequence(QKeySequence(currentKey));
        keyEdit->setObjectName(itemData.settingsKey);

        ui->table_Controls->setCellWidget(i, 1, keyEdit);
    }
}

void SettingsDialog::saveSettings() {

    SettingsManager::setValue("editor/defaultWidth", ui->spin_DefaultWidth->value());
    SettingsManager::setValue("editor/defaultHeight", ui->spin_DefaultHeight->value());
    SettingsManager::setValue("editor/undoLimit", ui->spin_UndoLimit->value());
    SettingsManager::setValue("editor/maxFrames", ui->spin_MaxFrames->value());
    SettingsManager::setValue("editor/maxLayers", ui->spin_MaxLayers->value());
    SettingsManager::setValue("editor/autoSave", ui->chk_AutoSave->isChecked());
    SettingsManager::setValue("editor/autoSaveInterval", ui->spin_AutoSaveInterval->value());
    SettingsManager::setValue("editor/rightClickEraser", ui->chk_RightClickEraser->isChecked());

    SettingsManager::setValue("export/defaultFormat", ui->cmb_Language->currentIndex());
    SettingsManager::setValue("export/variablePrefix", ui->input_VariablePrefix->text().trimmed());
    SettingsManager::setValue("export/byteFormat", ui->cmb_ByteFormat->currentIndex());
    SettingsManager::setValue("export/useProgmem", ui->chk_Progmem->isChecked());
    SettingsManager::setValue("export/autoCopy", ui->chk_AutoCopy->isChecked());

    SettingsManager::setTheme(ui->cmb_Theme->currentText());
    SettingsManager::setValue("ui/scale", ui->slider_Scale->value());
    SettingsManager::setValue("canvas/showGrid", ui->chk_ShowGrid->isChecked());
    SettingsManager::setValue("canvas/gridColor", currentGridColor);
    SettingsManager::setValue("canvas/bgStyle", ui->cmb_BgStyle->currentText());

    for (int i = 0; i < ui->table_Controls->rowCount(); ++i) {
        QWidget *widget = ui->table_Controls->cellWidget(i, 1);
        QKeySequenceEdit *keyEdit = qobject_cast<QKeySequenceEdit*>(widget);

        if (keyEdit) {
            QString key = keyEdit->objectName();
            QString sequence = keyEdit->keySequence().toString();
            SettingsManager::setValue(key, sequence);
        }
    }
}

void SettingsDialog::updateScaleLabel(int value) {
    ui->lbl_ScaleValue->setText(QString::number(value) + "%");
}

void SettingsDialog::chooseGridColor() {
    QColor initialColor(currentGridColor.isEmpty() ? "#333333" : currentGridColor);
    QColor newColor = QColorDialog::getColor(initialColor, this, tr("Select Grid Color"));

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
