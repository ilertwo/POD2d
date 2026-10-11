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
#include <QDockWidget>
#include <QFrame>
#include <QAbstractButton>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace {

class DisabledSymbolFilter : public QObject {
public:
    explicit DisabledSymbolFilter(QObject *parent) : QObject(parent) {}

    static void apply(QAbstractButton *b) {
        const QString on = b->property("symOn").toString();
        if (on.isEmpty()) return;
        const QString off = b->property("symOff").toString();
        b->setText((b->isEnabled() || off.isEmpty()) ? on : off);
    }
    static void attach(QAbstractButton *b, const QString &text) {
        static DisabledSymbolFilter *filter = new DisabledSymbolFilter(qApp);
        const bool arrow = (text == "<" || text == ">");
        b->setProperty("symOn", text);
        b->setProperty("symOff", arrow ? QString("X") : QString());
        if (!b->property("symFilter").toBool()) {
            b->installEventFilter(filter);
            b->setProperty("symFilter", true);
        }
        apply(b);
    }
    static void detach(QAbstractButton *b) {
        b->setProperty("symOn", QString());
    }

protected:
    bool eventFilter(QObject *obj, QEvent *e) override {
        if (e->type() == QEvent::EnabledChange) {
            if (auto *b = qobject_cast<QAbstractButton *>(obj)) apply(b);
        }
        return false;
    }
};

QWidget *makeLetterTitleBar(QDockWidget *dock) {
    auto *bar = new QFrame(dock);
    bar->setObjectName("dockLetterTitleBar");

    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(6, 2, 2, 2);
    lay->setSpacing(2);

    auto *title = new QLabel(dock->windowTitle(), bar);
    QObject::connect(dock, &QDockWidget::windowTitleChanged, title, &QLabel::setText);
    lay->addWidget(title, 1);

    auto makeBtn = [&](const QString &text, const QString &tip) {
        auto *b = new QPushButton(text, bar);
        b->setObjectName("dockTitleButton");
        b->setFixedSize(20, 20);
        b->setToolTip(tip);
        b->setFocusPolicy(Qt::NoFocus);
        lay->addWidget(b);
        return b;
    };

    QPushButton *floatBtn = makeBtn("^", "Float / Dock");
    QPushButton *closeBtn = makeBtn("X", "Close");

    QObject::connect(floatBtn, &QPushButton::clicked, dock, [dock]() { dock->setFloating(!dock->isFloating()); });
    QObject::connect(closeBtn, &QPushButton::clicked, dock, &QDockWidget::close);
    return bar;
}

bool hasLetterTitleBar(const QDockWidget *dock) {
    return dock->titleBarWidget() && dock->titleBarWidget()->objectName() == "dockLetterTitleBar";
}

void updateDockTitleBars(Ui::MainWindow *ui, const QString &theme) {
    const QList<QDockWidget *> docks = { ui->dockMiniMap, ui->dockTools, ui->dockLayers,
                                        ui->dockPalette, ui->dockFrames };
    for (QDockWidget *dock : docks) {
        if (theme == "1bit") {
            if (!hasLetterTitleBar(dock))
                dock->setTitleBarWidget(makeLetterTitleBar(dock));
        } else if (hasLetterTitleBar(dock)) {
            dock->setTitleBarWidget(nullptr);
        }
    }
}

}

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
            DisabledSymbolFilter::attach(btn, text);
        } else {
            DisabledSymbolFilter::detach(btn);
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

    updateDockTitleBars(ui, theme);
}
