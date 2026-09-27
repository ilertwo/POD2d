#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QColor>
#include <QTimer>

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void openFile(const QString &filePath);

protected:
    // System events override
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    // Start menu actions
    void addRecentProject(const QString &path);

    // File actions
    void onProjectReady(int width, int height, bool isRgb);
    void updateWindowTitle();
    void loadPalette();
    void savePalette();

    // Editor actions
    void clear();
    void undo();
    void redo();
    void chooseAndSetColor();
    void on_spin_brushSize_valueChanged(int value);
    void selectAll();

    void openExportMenu();
    void openSettings(int tabIndex = 0);
    void applySettings();
    void openPaletteEditor(int colorIndexToEdit = -1);

    // UI Updates
    void setEditorUIEnabled(bool enabled);
    void updateUIProportions(int projWidth, int projHeight);
    void updateRecentProjectsUI();
    void updateColorIndicators();
    void updateUiStates();

    bool maybeSave();
    void closeProject();

    // View toggles
    void setMiniMapVisible(bool visible);
    void setFrameListVisible(bool visible);
    void setLayerListVisible(bool visible);
    void setToolsVisible(bool visible);
    void setPaletteVisible(bool visible);

private:
    Ui::MainWindow *ui;

    // Main data model (Controller -> Model)
    ProjectModel *projectModel = nullptr;

    // Current file & Autosave
    FileController *fileController = nullptr;
    QTimer *autoSaveTimer = nullptr;

    // Color states
    QColor currentPrimaryColor = Qt::white;
    QColor currentSecondaryColor = Qt::black;

    // Initialization Stages (Startup)
    void initModels();
    void setupTheme();
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
};

#endif // MAINWINDOW_H
