#ifndef FILECONTROLLER_H
#define FILECONTROLLER_H

#include <QObject>
#include <QString>
#include <QImage>

class ProjectModel;
class QWidget;

class FileController : public QObject {
    Q_OBJECT
public:
    explicit FileController(ProjectModel *model, QWidget *parentWindow, QObject *parent = nullptr);

    void createProject();
    void openProject();
    void loadProjectFromFile(const QString &path);
    void saveProject();
    void saveProjectAs();

    void openPngAsProject(const QString &path);
    void actionImportPng();
    void importPngToCanvas(const QString &path);
    void actionImportCArray();

    QString getCurrentFilePath() const { return m_currentFilePath; }
    QString getCurrentProjectName() const { return m_currentProjectName; }
    int getProjectWidth() const { return m_projectWidth; }
    int getProjectHeight() const { return m_projectHeight; }
    bool getIsModified() const { return m_isModified; }

    void markModified();
    void resetState();

signals:
    void projectReady(int width, int height, bool isRgb);
    void projectSaved();
    void requestPasteToLayer();
    void stateChanged();
    void recentProjectAdded(const QString &path);

private:
    ProjectModel *m_model;
    QWidget *m_parentWindow;

    QString m_currentFilePath;
    QString m_currentProjectName;
    int m_projectWidth = 0;
    int m_projectHeight = 0;
    bool m_isModified = false;
};

#endif // FILECONTROLLER_H
