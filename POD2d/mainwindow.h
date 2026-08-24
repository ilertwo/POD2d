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
    void addRecentProject(const QString &path);

    // File actions
    void loadProjectFromFile(const QString &path);
    void saveProject();
    void saveProjectAs();
    void loadPalette();
    void savePalette();

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
    void openSettings(int tabIndex = 0);
    void openKeyBindings(int tabIndex = 1);
    void applySettings();
    void openPaletteEditor(int colorIndexToEdit = -1);

    void setEditorUIEnabled(bool enabled);
    void updateUIProportions(int projWidth, int projHeight);
    void updateRecentProjectsUI();
    void updateColorIndicators();
    void applyTheme(const QString &themeName);

    void markProjectAsModified();
    bool maybeSave();
    void closeProject();
    void closeEvent(QCloseEvent *event) override;

    void setMiniMapVisible(bool visible);
    void setFrameListVisible(bool visible);
    void setLayerListVisible(bool visible);
    void setToolsVisible(bool visible);

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWindow *ui;

    // Main data model (Controller -> Model)
    ProjectModel *projectModel = nullptr;

    // Additional windows
    QWidget *windowCreateProject = nullptr;

    // Color
    QColor currentPrimaryColor = Qt::white;
    QColor currentSecondaryColor = Qt::black;

    // Current file
    int projectWidth = 128;
    int projectHeight = 64;
    QString currentFilePath;
    QString currentProjectName;
    bool isProjectModified = false;
    QList<QColor> customPalette;
    QTimer *autoSaveTimer;

    // Initialization Stages (Startup)
    void initModels();
    void setupTheme();
    void setupWidgets();
    void loadIcons();
    void setupConnections();
    void setupPalette();

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
    void connectAutoSaveTimer();
    void connectRecentProjects();
    void connectActions();
    void connectFileActions();
    void connectEditActions();
    void connectViewActions();
    void connectPreferencesActions();
    void connectHelpActions();
    void setupShortcuts();

    // Interface update
    void rebuildLayersList();
    void rebuildFramesList();
    void rebuildPaletteGrid();
};

#endif // MAINWINDOW_H
