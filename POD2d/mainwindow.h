#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class ProjectModel;
class FileController;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Class for working with the UI
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void openFile(const QString &filePath);

private slots:
    // Start menu actions
    void buttonProjects();
    void buttonExamples();
    void recentProject();
    void buttonCreate();
    void buttonCancel();
    void addRecentProject(const QString &path);

    // File actions
    void onProjectReady(int width, int height, bool isRgb);
    void updateWindowTitle();
    void loadPalette();
    void savePalette();
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

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
    void applySettings();
    void openPaletteEditor(int colorIndexToEdit = -1);

    void setEditorUIEnabled(bool enabled);
    void updateUIProportions(int projWidth, int projHeight);
    void updateRecentProjectsUI();
    void updateColorIndicators();
    void applyTheme(const QString &themeName);
    QIcon generate1bitIcon(const QString &text);

    bool maybeSave();
    void closeProject();
    void closeEvent(QCloseEvent *event) override;

    void setMiniMapVisible(bool visible);
    void setFrameListVisible(bool visible);
    void setLayerListVisible(bool visible);
    void setToolsVisible(bool visible);
    void setPaletteVisible(bool visible);

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
    FileController *fileController = nullptr;
    QTimer *autoSaveTimer;

    // Initialization Stages (Startup)
    void initModels();
    void setupTheme();
    void loadIcons();
    void setupConnections();
    void setupPalette();

    // Signal connection assignment
    void connectModelToLists();
    void connectMenuButtons();
    void connectEditorControls();
    void connectDrawingTools();
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
};

#endif // MAINWINDOW_H
