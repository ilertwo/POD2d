#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QLine>
#include <QPoint>
#include <QImage>
#include <QColor>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QString>
#include <fstream>
#include <QColorDialog>

#include "projectmodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

//Клас для роботи з UI створений Qt
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //конструктор
    MainWindow(QWidget *parent = nullptr);
    //деструктор
    ~MainWindow();

    void createProject();
    void openProject();
    void buttonProjects();
    void buttonExamples();
    void recentProject();

    void buttonCreate();
    void buttonCancel();

    void setCurrentLayer();
    void addLayer();
    void deleteCurrentLayer();
    void clear();
    void undo();
    void redo();
    void setScale(int newScale);
    void chooseAndSetColor();
    void on_spin_brushSize_valueChanged(int value);

    void openExportMenu();


private:

    Ui::MainWindow *ui;
    QGraphicsScene *scene;

    ProjectModel *projectModel;

    void rebuildLayersList();

    QWidget* windowCreateProject = nullptr;
    int scaleFactor = 5;
    QImage canvasImage;

private:
    void initModels();
    void setupWidgets();
    void loadIcons();
    void setupConnections();

    void setupLayersListWidget();
    void setupFramesListWidget();

    void connectModelToLists();
    void connectMenuButtons();
    void connectEditorControls();
    void connectDrawingTools();

    void connectFramesList();
    void connectLayersList();
    void connectMiniCanvas();

    void connectPlayerControls();

    void rebuildFramesList();
};
#endif // MAINWINDOW_H
