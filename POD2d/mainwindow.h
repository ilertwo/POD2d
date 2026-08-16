#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class ProjectModel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Class for working with the UI
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Start menu actions
    void createProject();
    void openProject();
    void buttonProjects();
    void buttonExamples();
    void recentProject();
    void buttonCreate();
    void buttonCancel();

    // File actions
    void saveProject();
    void saveProjectAs();

    // Editor actions
    void addLayer();
    void deleteCurrentLayer();
    void clear();
    void undo();
    void redo();
    void setScale(int newScale);
    void chooseAndSetColor();
    void on_spin_brushSize_valueChanged(int value);
    void selectAll();

    void openExportMenu();

    void setEditorUIEnabled(bool enabled);

    void markProjectAsModified();
    bool maybeSave();
    void closeProject();
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

    // Main data model (Controller -> Model)
    ProjectModel *projectModel = nullptr;

    // Additional windows
    QWidget *windowCreateProject = nullptr;

    // Current file
    QString currentFilePath;
    QString currentProjectName;
    bool isProjectModified = false;

    // Initialization Stages (Startup)
    void initModels();
    void setupWidgets();
    void loadIcons();
    void setupConnections();
    void setupShortcuts();

    void setupLayersListWidget();
    void setupFramesListWidget();

    // Signal connection assignment
    void connectModelToLists();
    void connectMenuButtons();
    void connectEditorControls();
    void connectDrawingTools();
    void connectFramesList();
    void connectLayersList();
    void connectMiniCanvas();
    void connectPlayerControls();

    // Interface update
    void rebuildLayersList();
    void rebuildFramesList();
};

#endif // MAINWINDOW_H
