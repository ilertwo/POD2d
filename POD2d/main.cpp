#include "mainwindow.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    if (argc > 1) {
        QString filePath = QString::fromUtf8(argv[1]);
        if (!filePath.isEmpty()) {
            QTimer::singleShot(100, &w, [&w, filePath]() {
                w.loadProjectFromFile(filePath);
            });
        }
    }

    return a.exec();
}
