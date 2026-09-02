#include "palettedialog.h"
#include "ui_palettedialog.h"

#include <QMouseEvent>
#include <QPainter>
#include <QGridLayout>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QRegularExpression>
#include <QLineEdit>
#include <QSettings>

PaletteDialog::PaletteDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PaletteDialog),
    m_activeIndex(-1)
{
    ui->setupUi(this);
    this->setWindowTitle("Palette Editor");

    ui->lbl_Hue->installEventFilter(this);
    ui->lbl_Sat->installEventFilter(this);
    ui->lbl_Val->installEventFilter(this);

    ui->lbl_Hue->setCursor(Qt::CrossCursor);
    ui->lbl_Sat->setCursor(Qt::CrossCursor);
    ui->lbl_Val->setCursor(Qt::CrossCursor);

    connect(ui->btn_Ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->lineEdit_Hex, &QLineEdit::returnPressed, this, [this]() {
        QString hex = ui->lineEdit_Hex->text().trimmed();
        if (!hex.startsWith("#")) {
            hex.prepend("#");
        }

        if (QColor::isValidColor(hex)) {
            m_currentColor = QColor(hex);
            syncColorToUI();
        } else {
            updateHex();
        }
    });

    auto updateFromSpins = [this]() {
        m_currentColor.setHsv(ui->spin_Hue->value(), ui->spin_Sat->value(), ui->spin_Val->value());
        syncColorToUI(false);
    };
    connect(ui->spin_Hue, QOverload<int>::of(&QSpinBox::valueChanged), this, updateFromSpins);
    connect(ui->spin_Sat, QOverload<int>::of(&QSpinBox::valueChanged), this, updateFromSpins);
    connect(ui->spin_Val, QOverload<int>::of(&QSpinBox::valueChanged), this, updateFromSpins);

    setTheme();
}

void PaletteDialog::setTheme() {
    QString basePath = QFileInfo(__FILE__).dir().absolutePath();
    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

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

    setBtnIcon(ui->btn_Load, "L", "load.png");
    setBtnIcon(ui->btn_Save, "S", "save.png");
}

PaletteDialog::~PaletteDialog() {
    delete ui;
}

void PaletteDialog::setPaletteData(const QList<QColor> &palette, int activeIndex) {
    m_palette = palette;
    m_activeIndex = activeIndex;

    if (m_activeIndex >= 0 && m_activeIndex < m_palette.size()) {
        m_currentColor = m_palette[m_activeIndex];
    } else {
        m_currentColor = QColor(Qt::red);
    }

    rebuildGrid();
    syncColorToUI();
}

QList<QColor> PaletteDialog::getPalette() const {
    return m_palette;
}

// INTERFACE AND GRADIENT LOGIC
void PaletteDialog::syncColorToUI(bool updateSpins) {
    updateSpectrums();
    updateHex();

    if (updateSpins) {
        ui->spin_Hue->blockSignals(true);
        ui->spin_Sat->blockSignals(true);
        ui->spin_Val->blockSignals(true);

        ui->spin_Hue->setValue(qMax(0, m_currentColor.hue()));
        ui->spin_Sat->setValue(m_currentColor.saturation());
        ui->spin_Val->setValue(m_currentColor.value());

        ui->spin_Hue->blockSignals(false);
        ui->spin_Sat->blockSignals(false);
        ui->spin_Val->blockSignals(false);
    }

    if (m_activeIndex >= 0 && m_activeIndex < m_palette.size()) {
        m_palette[m_activeIndex] = m_currentColor;
        rebuildGrid();
    }
}

void PaletteDialog::updateHex() {
    ui->lineEdit_Hex->setText(m_currentColor.name().toUpper());
}

void PaletteDialog::updateSpectrums() {
    int h = qMax(0, m_currentColor.hue());
    int s = m_currentColor.saturation();
    int v = m_currentColor.value();

    auto drawMarker = [](QPixmap &pixmap, int x) {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setPen(QPen(Qt::black, 3));
        painter.drawEllipse(QPoint(x, pixmap.height() / 2), 5, 5);

        painter.setPen(QPen(Qt::white, 1));
        painter.drawEllipse(QPoint(x, pixmap.height() / 2), 4, 4);
    };

    // 1. Hue
    QImage imgHue(360, 1, QImage::Format_RGB32);
    for (int i = 0; i < 360; ++i) {
        imgHue.setPixelColor(i, 0, QColor::fromHsv(i, 255, 255));
    }
    QPixmap pixHue = QPixmap::fromImage(imgHue).scaled(ui->lbl_Hue->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    int xHue = (h * (ui->lbl_Hue->width() - 1)) / 359;
    drawMarker(pixHue, xHue);
    ui->lbl_Hue->setPixmap(pixHue);

    // 2. Saturation
    QImage imgSat(256, 1, QImage::Format_RGB32);
    for (int i = 0; i < 256; ++i) {
        imgSat.setPixelColor(i, 0, QColor::fromHsv(h, i, v));
    }
    QPixmap pixSat = QPixmap::fromImage(imgSat).scaled(ui->lbl_Sat->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    int xSat = (s * (ui->lbl_Sat->width() - 1)) / 255;
    drawMarker(pixSat, xSat);
    ui->lbl_Sat->setPixmap(pixSat);

    // 3. Value
    QImage imgVal(256, 1, QImage::Format_RGB32);
    for (int i = 0; i < 256; ++i) {
        imgVal.setPixelColor(i, 0, QColor::fromHsv(h, s, i));
    }
    QPixmap pixVal = QPixmap::fromImage(imgVal).scaled(ui->lbl_Val->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    int xVal = (v * (ui->lbl_Val->width() - 1)) / 255;
    drawMarker(pixVal, xVal);
    ui->lbl_Val->setPixmap(pixVal);
}

void PaletteDialog::rebuildGrid() {
    QGridLayout *gridLayout = qobject_cast<QGridLayout*>(ui->widget_Palette->layout());

    if (!gridLayout) {
        if (ui->widget_Palette->layout()) delete ui->widget_Palette->layout();
        gridLayout = new QGridLayout(ui->widget_Palette);
        gridLayout->setSpacing(2);
        gridLayout->setContentsMargins(0, 0, 0, 0);
    }

    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    int columns = 6;

    for (int i = 0; i < m_palette.size(); ++i) {
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(24, 24);
        btn->setCursor(Qt::PointingHandCursor);

        QString border = (i == m_activeIndex) ? "border: 2px solid white;" : "border: 1px solid #555;";
        btn->setStyleSheet(QString("background-color: %1; %2 border-radius: 2px;").arg(m_palette[i].name(), border));

        btn->setProperty("colorIndex", i);
        btn->installEventFilter(this);

        gridLayout->addWidget(btn, i / columns, i % columns);
    }

    QPushButton *addBtn = new QPushButton("+");
    addBtn->setFixedSize(24, 24);
    addBtn->setStyleSheet("background-color: #333; color: white; border: 1px dashed #777; border-radius: 2px;");
    addBtn->setCursor(Qt::PointingHandCursor);

    connect(addBtn, &QPushButton::clicked, this, [this]() {
        m_palette.append(m_currentColor);
        m_activeIndex = m_palette.size() - 1;
        rebuildGrid();
    });

    gridLayout->addWidget(addBtn, m_palette.size() / columns, m_palette.size() % columns);
}

// MESH PROCESSING (INTERACTION WITH GRADIENTS AND THE GRID)
bool PaletteDialog::eventFilter(QObject *watched, QEvent *event) {

    if (watched->parent() == ui->widget_Palette && event->type() == QEvent::MouseButtonPress) {
        QPushButton *btn = qobject_cast<QPushButton*>(watched);
        if (btn && btn->property("colorIndex").isValid()) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            int index = btn->property("colorIndex").toInt();

            if (me->button() == Qt::LeftButton) {
                m_activeIndex = index;
                m_currentColor = m_palette[index];
                syncColorToUI();
            } else if (me->button() == Qt::MiddleButton) {
                m_palette.removeAt(index);
                if (m_activeIndex == index) m_activeIndex = -1;
                rebuildGrid();
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->buttons() & Qt::LeftButton) {

            if (watched == ui->lbl_Hue) {
                int x = qBound(0, me->pos().x(), ui->lbl_Hue->width() - 1);
                float ratio = (float)x / (ui->lbl_Hue->width() - 1);
                int h = qBound(0, (int)(ratio * 359), 359);
                m_currentColor.setHsv(h, m_currentColor.saturation(), m_currentColor.value());
                syncColorToUI();
                return true;
            }
            else if (watched == ui->lbl_Sat) {
                int x = qBound(0, me->pos().x(), ui->lbl_Sat->width() - 1);
                float ratio = (float)x / (ui->lbl_Sat->width() - 1);
                int s = qBound(0, (int)(ratio * 255), 255);
                int h = qMax(0, m_currentColor.hue());
                m_currentColor.setHsv(h, s, m_currentColor.value());
                syncColorToUI();
                return true;
            }
            else if (watched == ui->lbl_Val) {
                int x = qBound(0, me->pos().x(), ui->lbl_Val->width() - 1);
                float ratio = (float)x / (ui->lbl_Val->width() - 1);
                int v = qBound(0, (int)(ratio * 255), 255);
                int h = qMax(0, m_currentColor.hue());
                m_currentColor.setHsv(h, m_currentColor.saturation(), v);
                syncColorToUI();
                return true;
            }
        }
    }

    return QDialog::eventFilter(watched, event);
}
