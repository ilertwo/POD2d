#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QColor>

class SettingsManager {
public:
    static QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant());
    static void setValue(const QString &key, const QVariant &value);

    // ==========================================
    // UI & View
    // ==========================================
    static QString getTheme();
    static void setTheme(const QString &theme);
    static int getScale();

    static bool getShowLayers();
    static void setShowLayers(bool show);
    static bool getShowFrames();
    static void setShowFrames(bool show);
    static bool getShowTools();
    static void setShowTools(bool show);
    static bool getShowMiniMap();
    static void setShowMiniMap(bool show);

    // ==========================================
    // Canvas
    // ==========================================
    static bool getShowGrid();
    static QColor getGridColor();
    static QString getBgStyle();
    static QColor getMonoColor();
    static void setMonoColor(const QColor &color);

    // ==========================================
    // Editor Tools
    // ==========================================
    static bool getAutoSave();
    static int getAutoSaveInterval();
    static bool getRightClickEraser();
    static int getUndoLimit();
    static int getMaxFrames();
    static int getMaxLayers();

    // ==========================================
    // Files & Projects
    // ==========================================
    static QStringList getRecentProjects();
    static void addRecentProject(const QString &path);

    static QString getLastDirectory();
    static void setLastDirectory(const QString &dir);

    static int getExportByteFormat();
};

#endif // SETTINGSMANAGER_H
