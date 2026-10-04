#include "mainwindow.h"
#include "logger.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("POD2d");
    QCoreApplication::setOrganizationName("Ilertwo");

    Logger::init();

    MainWindow w;
    w.show();

    if (argc > 1) {
        QString filePath = QString::fromUtf8(argv[1]);
        if (!filePath.isEmpty()) {
            QTimer::singleShot(100, &w, [&w, filePath]() {
                w.openFile(filePath);
            });
        }
    }

    return a.exec();
}
