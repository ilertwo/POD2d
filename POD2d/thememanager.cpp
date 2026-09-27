#include "thememanager.h"
#include "ui_mainwindow.h"
#include "projectmodel.h"
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QPainter>

void ThemeManager::applyTheme(const QString &themeName, QWidget *mainWindow) {
    QString basePath = QFileInfo(__FILE__).dir().absolutePath();
    QString filePath = QString(basePath + "/themes/%1.qss").arg(themeName);

    QFile file(filePath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        QString styleSheet = stream.readAll();
        styleSheet.replace("{BASE_PATH}", basePath);
        qApp->setStyleSheet(styleSheet);
        file.close();

        QSettings settings("POD2d", "EditorSettings");
        settings.setValue("ui/theme", themeName);
    }
}

QIcon ThemeManager::generate1bitIcon(const QString &text) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setPen(Qt::white);
    QFont font("Courier New", 14, QFont::Bold);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, text);

    return QIcon(pixmap);
}

void ThemeManager::loadIcons(Ui::MainWindow *ui, const QString &theme, ProjectModel *model, QWidget *mainWindow) {
    QString basePath = QFileInfo(__FILE__).dir().absolutePath();
    QFont pixelFont("Courier New", 14, QFont::Bold);

    auto setBtnIcon = [&](QPushButton* btn, const QString& text, const QString& iconName) {
        if (theme == "1bit") {
            btn->setIcon(QIcon());
            btn->setText(text);
            btn->setFont(pixelFont);
        } else {
            btn->setText("");
            QString fullPath = basePath + "/image/" + theme + "/" + iconName;
            btn->setIcon(QIcon(fullPath));
        }
    };

    mainWindow->setWindowIcon(QIcon(basePath + "/image/" + "POD2d_icon.png"));

    QPixmap logoPixmap = (theme == "light") ?
                             QPixmap(basePath + "/image/POD2d_icon_white.png") :
                             QPixmap(basePath + "/image/POD2d_icon.png");

    QPixmap scaledLogo = logoPixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->lbl_Logo->setPixmap(scaledLogo);

    setBtnIcon(ui->btn_Save, "S", "save.png");
    setBtnIcon(ui->btn_Undo, "<", "undo.png");
    setBtnIcon(ui->btn_Redo, ">", "redo.png");

    setBtnIcon(ui->btn_RectangleSelection, "[:]", "select.png");
    setBtnIcon(ui->btn_LassoSelection, "@", "lasso.png");
    setBtnIcon(ui->btn_ShapeSelection, "!", "shape.png");
    setBtnIcon(ui->btn_Lighten, "*", "lighten.png");

    setBtnIcon(ui->btn_Pen, "/", "pen.png");
    setBtnIcon(ui->btn_Eraser, "=", "eraser.png");
    setBtnIcon(ui->btn_Pipette, "j", "pipette.png");
    setBtnIcon(ui->btn_Dithering, "#", "dithering.png");
    setBtnIcon(ui->btn_Fill, "U", "pain.png");
    setBtnIcon(ui->btn_Text, "A", "text.png");
    setBtnIcon(ui->btn_Line, "\\", "line.png");
    setBtnIcon(ui->btn_BrokenLine, "N", "brokenLine.png");
    setBtnIcon(ui->btn_Circle, "O", "circle.png");
    setBtnIcon(ui->btn_Rectangle, "[]", "rectangle.png");

    setBtnIcon(ui->btn_Pan, "W", "pan.png");
    setBtnIcon(ui->btn_Rotate, "G", "rotate.png");
    setBtnIcon(ui->btn_VerticalMiror, "|", "mirror.png");
    setBtnIcon(ui->btn_CenterView, "+", "center.png");

    setBtnIcon(ui->btn_HideMiniMap, "m", "hide_minimap.png");
    setBtnIcon(ui->btn_HideFrames, "f", "hide_frames.png");
    setBtnIcon(ui->btn_HideLayers, "l", "hide_layers.png");
    setBtnIcon(ui->btn_EditMode, "E", "edit_mode.png");

    bool isPlaying = model && model->isPlaying();
    if (isPlaying) {
        setBtnIcon(ui->btn_Play, "X", "stop.png");
    } else {
        setBtnIcon(ui->btn_Play, ">", "play.png");
    }
}
