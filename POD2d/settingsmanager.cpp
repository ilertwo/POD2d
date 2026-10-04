#include "settingsmanager.h"
#include <QSettings>
#include <QStandardPaths>

namespace {
const QString ORG_NAME = "POD2d";
const QString APP_NAME = "EditorSettings";

QSettings getSettings() {
    return QSettings(ORG_NAME, APP_NAME);
}
}

QVariant SettingsManager::getValue(const QString &key, const QVariant &defaultValue) {
    return getSettings().value(key, defaultValue);
}

void SettingsManager::setValue(const QString &key, const QVariant &value) {
    getSettings().setValue(key, value);
}

// UI
QString SettingsManager::getTheme() { return getValue("ui/theme", "dark").toString(); }
void SettingsManager::setTheme(const QString &theme) { setValue("ui/theme", theme); }
int SettingsManager::getScale() { return getValue("ui/scale", 100).toInt(); }

bool SettingsManager::getShowLayers() { return getValue("ui/showLayers", true).toBool(); }
void SettingsManager::setShowLayers(bool show) { setValue("ui/showLayers", show); }

bool SettingsManager::getShowFrames() { return getValue("ui/showFrames", true).toBool(); }
void SettingsManager::setShowFrames(bool show) { setValue("ui/showFrames", show); }

bool SettingsManager::getShowTools() { return getValue("ui/showTools", true).toBool(); }
void SettingsManager::setShowTools(bool show) { setValue("ui/showTools", show); }

bool SettingsManager::getShowMiniMap() { return getValue("ui/showMiniMap", true).toBool(); }
void SettingsManager::setShowMiniMap(bool show) { setValue("ui/showMiniMap", show); }

// Canvas
bool SettingsManager::getShowGrid() { return getValue("canvas/showGrid", true).toBool(); }
QColor SettingsManager::getGridColor() { return QColor(getValue("canvas/gridColor", "#333333").toString()); }
QString SettingsManager::getBgStyle() { return getValue("canvas/bgStyle", "Checkerboard").toString(); }
QColor SettingsManager::getMonoColor() { return QColor(getValue("editor/monoColor", "#FFFFFF").toString()); }
void SettingsManager::setMonoColor(const QColor &color) { setValue("editor/monoColor", color.name()); }

// Editor
bool SettingsManager::getAutoSave() { return getValue("editor/autoSave", false).toBool(); }
int SettingsManager::getAutoSaveInterval() { return getValue("editor/autoSaveInterval", 5).toInt(); }
bool SettingsManager::getRightClickEraser() { return getValue("editor/rightClickEraser", false).toBool(); }
int SettingsManager::getUndoLimit() { return getValue("editor/undoLimit", 50).toInt(); }
int SettingsManager::getMaxFrames() { return getValue("editor/maxFrames", 64).toInt(); }
int SettingsManager::getMaxLayers() { return getValue("editor/maxLayers", 16).toInt(); }

// Files
QStringList SettingsManager::getRecentProjects() {
    return getValue("recentProjects").toStringList();
}

void SettingsManager::addRecentProject(const QString &path) {
    QStringList recent = getRecentProjects();
    recent.removeAll(path);
    recent.prepend(path);
    if (recent.size() > 10) {
        recent.removeLast();
    }
    setValue("recentProjects", recent);
}

QString SettingsManager::getLastDirectory() {
    return getValue("lastDirectory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();
}

void SettingsManager::setLastDirectory(const QString &dir) {
    setValue("lastDirectory", dir);
}

int SettingsManager::getExportByteFormat() {
    return getValue("export/byteFormat", 0).toInt();
}
