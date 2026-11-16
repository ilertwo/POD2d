#include "mainwindow.h"
#include "logger.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QSettings>
#include <QDir>

#ifdef Q_OS_WIN
#include <shlobj.h>
#endif

static void registerFileAssociations() {
#ifdef Q_OS_WIN
    const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString imgProgId = "POD2d.ImageFile";
    const QString projProgId = "POD2d.ProjectFile";

    QSettings reg("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);

    // 1. Формат проєкту .pod2d
    reg.setValue(projProgId + "/Default", "POD2d Project");
    reg.setValue(projProgId + "/FriendlyTypeName", "POD2d Pixel Art Project");
    reg.setValue(projProgId + "/DefaultIcon/Default", "\"" + appPath + "\",0");
    reg.setValue(projProgId + "/shell/open/command/Default", "\"" + appPath + "\" \"%1\"");
    reg.setValue(".pod2d/Default", projProgId);
    reg.setValue(".pod2d/OpenWithProgids/" + projProgId, QString());

    // 2. Зображення .png та .gif
    reg.setValue(imgProgId + "/Default", "POD2d Image");
    reg.setValue(imgProgId + "/FriendlyTypeName", "POD2d Pixel Art Editor");
    reg.setValue(imgProgId + "/DefaultIcon/Default", "\"" + appPath + "\",0");
    reg.setValue(imgProgId + "/shell/open/command/Default", "\"" + appPath + "\" \"%1\"");

    const QStringList imgExts = { ".png", ".gif" };
    for (const QString &ext : imgExts) {
        reg.setValue(ext + "/OpenWithProgids/" + imgProgId, QString());
    }

    // 3. Реєстрація в системних налаштуваннях Windows ("Програми за замовчуванням")
    QSettings appReg("HKEY_CURRENT_USER\\Software\\Ilertwo\\POD2d\\Capabilities", QSettings::NativeFormat);
    appReg.setValue("ApplicationName", "POD2d");
    appReg.setValue("ApplicationDescription", "Pixel Art & Embedded Display Graphics Editor");
    appReg.setValue("FileAssociations/.pod2d", projProgId);
    for (const QString &ext : imgExts) {
        appReg.setValue("FileAssociations/" + ext, imgProgId);
    }

    QSettings regApps("HKEY_CURRENT_USER\\Software\\RegisteredApplications", QSettings::NativeFormat);
    regApps.setValue("POD2d", "Software\\Ilertwo\\POD2d\\Capabilities");

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
#endif
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("POD2d");
    QCoreApplication::setOrganizationName("Ilertwo");

    Logger::init();
    registerFileAssociations();

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("file", "Image (.png, .gif) or project (.pod2d) file to open.");
    parser.process(a);

    const QStringList args = parser.positionalArguments();
    const QString fileToOpen = args.isEmpty() ? QString() : args.first();

    MainWindow w;
    w.show();

    if (!fileToOpen.isEmpty()) {
        QTimer::singleShot(100, &w, [&w, fileToOpen]() {
            w.openFile(fileToOpen);
        });
    }

    return a.exec();
}
