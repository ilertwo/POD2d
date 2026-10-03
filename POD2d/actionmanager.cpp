#include "actionmanager.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <QAction>
#include <QPushButton>
#include <QKeySequence>

void ActionManager::setupShortcuts(Ui::MainWindow *ui) {
    QSettings settings("POD2d", "EditorSettings");

    auto setShortcutAndToolTip = [&](auto* element, const QString& tooltipName, const QString& settingsKey, const QString& defaultShortcut) {
        if (!element) return;

        QString shortcutStr = settings.value(settingsKey, defaultShortcut).toString();
        QKeySequence sequence(shortcutStr);

        element->setShortcut(sequence);

        if (shortcutStr.isEmpty()) {
            element->setToolTip(tooltipName);
        } else {
            element->setToolTip(QString("%1 (%2)").arg(tooltipName, sequence.toString(QKeySequence::NativeText)));
        }
    };

    // File (QAction)
    setShortcutAndToolTip(ui->act_NewFile, "New Project", "shortcuts/newFile", "Ctrl+N");
    setShortcutAndToolTip(ui->act_Save, "Save", "shortcuts/save", "Ctrl+S");
    setShortcutAndToolTip(ui->act_SaveAs, "Save As", "shortcuts/saveAs", "Ctrl+Shift+S");
    setShortcutAndToolTip(ui->act_OpenFile, "Open", "shortcuts/openFile", "Ctrl+O");
    setShortcutAndToolTip(ui->act_Close, "Close Project", "shortcuts/close", "Ctrl+W");
    setShortcutAndToolTip(ui->act_Exit, "Exit", "shortcuts/exit", "Alt+F4");
    setShortcutAndToolTip(ui->act_ExportCode, "Export", "shortcuts/export", "Ctrl+E");

    // Edit (QAction)
    setShortcutAndToolTip(ui->act_Undo, "Undo", "shortcuts/undo", "Ctrl+Z");
    setShortcutAndToolTip(ui->act_Redo, "Redo", "shortcuts/redo", "Ctrl+Y");
    setShortcutAndToolTip(ui->act_Copy, "Copy", "shortcuts/copy", "Ctrl+C");
    setShortcutAndToolTip(ui->act_Cut, "Cut", "shortcuts/cut", "Ctrl+X");
    setShortcutAndToolTip(ui->act_Paste, "Paste", "shortcuts/paste", "Ctrl+V");
    setShortcutAndToolTip(ui->act_Select, "Select All", "shortcuts/selectAll", "Ctrl+A");
    setShortcutAndToolTip(ui->act_Clear, "Clear Canvas", "shortcuts/clear", "Delete");

    // Drawing tools
    setShortcutAndToolTip(ui->btn_Pen, "Pen Tool", "shortcuts/pen", "P");
    setShortcutAndToolTip(ui->btn_Eraser, "Eraser Tool", "shortcuts/eraser", "E");
    setShortcutAndToolTip(ui->btn_Fill, "Fill Tool", "shortcuts/fill", "F");
    setShortcutAndToolTip(ui->btn_Line, "Line Tool", "shortcuts/line", "L");
    setShortcutAndToolTip(ui->btn_Rectangle, "Rectangle", "shortcuts/rectangle", "R");
    setShortcutAndToolTip(ui->btn_Circle, "Circle", "shortcuts/circle", "C");
    setShortcutAndToolTip(ui->btn_Text, "Text Tool", "shortcuts/text", "T");
    setShortcutAndToolTip(ui->btn_Dithering, "Dithering Tool", "shortcuts/dithering", "D");
    setShortcutAndToolTip(ui->btn_BrokenLine, "Broken Line Tool", "shortcuts/brokenLine", "B");
    setShortcutAndToolTip(ui->btn_Pipette, "Pipette Tool", "shortcuts/pipette", "I");

    // Navigation and selection
    setShortcutAndToolTip(ui->btn_Pan, "Pan Tool", "shortcuts/pan", "Space");
    setShortcutAndToolTip(ui->btn_LassoSelection, "Lasso Selection", "shortcuts/lassoSelection", "S");
    setShortcutAndToolTip(ui->btn_RectangleSelection, "Selection", "shortcuts/select", "");
    setShortcutAndToolTip(ui->btn_ShapeSelection, "Shape Selection", "shortcuts/shapeSelection", "");

    // Modifiers & Views
    setShortcutAndToolTip(ui->btn_Rotate, "Rotate", "shortcuts/rotate", "");
    setShortcutAndToolTip(ui->btn_Lighten, "Lighten", "shortcuts/lighten", "");
    setShortcutAndToolTip(ui->btn_VerticalMiror, "Vertical Miror", "shortcuts/verticalMiror", "");
    setShortcutAndToolTip(ui->btn_CenterView, "Center View", "shortcuts/centerView", "");

    // Frames and Layers
    setShortcutAndToolTip(ui->btn_AddFrame, "Add Frame", "shortcuts/addFrame", "Ctrl+N");
    setShortcutAndToolTip(ui->btn_DeleteFrame, "Delete Frame", "shortcuts/deleteFrame", "Ctrl+Shift+D");
    setShortcutAndToolTip(ui->btn_AddLayer, "Add Layer", "shortcuts/addLayer", "Ctrl+Shift+N");
    setShortcutAndToolTip(ui->btn_DeleteLayer, "Delete Layer", "shortcuts/deleteLayer", "Ctrl+Alt+D");

    // Player & UI Toggles
    setShortcutAndToolTip(ui->btn_Play, "Play Animation", "shortcuts/play", "Return");
    setShortcutAndToolTip(ui->btn_HideMiniMap, "Mini Map", "shortcuts/miniMap", "");
    setShortcutAndToolTip(ui->btn_HideFrames, "Frames", "shortcuts/hideFrames", "");
    setShortcutAndToolTip(ui->btn_HideLayers, "Layers", "shortcuts/hideLayers", "");
    setShortcutAndToolTip(ui->btn_EditMode, "Edit Mode", "shortcuts/editMode", "");
}
