#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QIcon>

class QWidget;
namespace Ui { class MainWindow; }
class ProjectModel;

class ThemeManager {
public:
    static void applyTheme(const QString &themeName, QWidget *mainWindow);
    static void loadIcons(Ui::MainWindow *ui, const QString &theme, ProjectModel *model, QWidget *mainWindow);
    static QIcon generate1bitIcon(const QString &text);
};

#endif // THEMEMANAGER_H
