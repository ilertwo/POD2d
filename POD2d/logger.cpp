#include "logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QMutex>
#include <QDebug>

namespace {
QFile* logFile = nullptr;
QMutex logMutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QMutexLocker locker(&logMutex);

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString typeStr;

    switch (type) {
    case QtDebugMsg:    typeStr = "[DEBUG]"; break;
    case QtInfoMsg:     typeStr = "[INFO ]"; break;
    case QtWarningMsg:  typeStr = "[WARN ]"; break;
    case QtCriticalMsg: typeStr = "[CRIT ]"; break;
    case QtFatalMsg:    typeStr = "[FATAL]"; break;
    }

    QString logMessage = QString("%1 %2 %3").arg(timeStr, typeStr, msg);

    QTextStream console(stdout);
    console << logMessage << "\n";

    if (logFile && logFile->isOpen()) {
        QTextStream out(logFile);
        out << logMessage << "\n";
        out.flush();
    }

    if (type == QtFatalMsg) {
        abort();
    }
}
}

void Logger::init() {
    QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir dir(logDirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString log0 = logDirPath + "/pod2d.log";
    QString log1 = logDirPath + "/pod2d_1.log";
    QString log2 = logDirPath + "/pod2d_2.log";
    QString log3 = logDirPath + "/pod2d_3.log";

    if (QFile::exists(log3)) QFile::remove(log3);
    if (QFile::exists(log2)) QFile::rename(log2, log3);
    if (QFile::exists(log1)) QFile::rename(log1, log2);
    if (QFile::exists(log0)) QFile::rename(log0, log1);

    logFile = new QFile(log0);
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qInstallMessageHandler(customMessageHandler);

        qInfo() << "=====================================";
        qInfo() << "POD2d Session Started";
        qInfo() << "Log directory:" << logDirPath;
        qInfo() << "=====================================";
    }
}
