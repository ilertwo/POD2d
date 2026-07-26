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



//#include "engine.h"

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


    //Сетер
    void setScale(float s);
    //Гетери
    std::string getScale();

    void createProject();
    void openProject();
    void buttonProjects();
    void buttonExemples();
    void recentProject();

    void buttonCreate();
    void buttonCancel();

    void saveInTxt();
    void printCode();
    void moveLayer();
    void prevLayer();
    void nextLayer();
    void allLayer();
    void clear();
    void undo();
    void redo();
    void setScale(int newScale);
    void showImage();
    void openFrameCreateProject();
    void closeFrameCreateProject();
    void on_chooseColorButton_clicked();
    //методи для кнопок для коректної роботи програмии
    void showMenu();    //метод який показує/ховає меню додавання трапеції

    //запис в файл
    void createLogFile();//ініціалізація лог файлу
    //void writeToLog(Figures* f, int index);//додавання записів

    //допоміжні методи
    //void update();//оновлення зображення на сцені

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;

    QWidget* windowCreateProject = nullptr;
    int scaleFactor = 5;
    QImage canvasImage;
};
#endif // MAINWINDOW_H
