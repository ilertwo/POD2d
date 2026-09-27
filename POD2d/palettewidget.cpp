#include "palettewidget.h"
#include <QGridLayout>
#include <QPushButton>
#include <QSettings>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>

PaletteWidget::PaletteWidget(QWidget *parent) : QWidget(parent) {
    m_gridLayout = new QGridLayout(this);
    m_gridLayout->setSpacing(0);
    setLayout(m_gridLayout);

    loadPaletteFromSettings();
}

void PaletteWidget::loadPaletteFromSettings() {
    QSettings settings("POD2d", "EditorSettings");
    QStringList savedColors = settings.value("customPalette").toStringList();

    m_palette.clear();
    if (savedColors.isEmpty()) {
        m_palette = { QColor(0, 0, 0), QColor(255, 255, 255), QColor(255, 0, 0), QColor(0, 255, 0), QColor(0, 0, 255), QColor(255, 255, 0) };
    } else {
        for (const QString& hex : savedColors) {
            m_palette.append(QColor(hex));
        }
    }
    rebuildGrid();
}

void PaletteWidget::savePaletteToSettings() {
    QSettings settings("POD2d", "EditorSettings");
    QStringList hexColors;
    for (const QColor& c : m_palette) {
        hexColors.append(c.name(QColor::HexArgb));
    }
    settings.setValue("customPalette", hexColors);
}

void PaletteWidget::setPalette(const QList<QColor> &palette) {
    m_palette = palette;
    rebuildGrid();
    savePaletteToSettings();
}

QList<QColor> PaletteWidget::getPalette() const {
    return m_palette;
}

void PaletteWidget::reloadTheme() {
    rebuildGrid();
}

void PaletteWidget::rebuildGrid() {
    QLayoutItem *child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

    if (theme == "1bit") {
        m_gridLayout->setContentsMargins(5, 5, 5, 5);
    } else {
        m_gridLayout->setContentsMargins(0, 0, 0, 0);
    }
    m_gridLayout->setAlignment(Qt::AlignTop);

    int columns = 4;
    for (int i = 0; i < columns; ++i) m_gridLayout->setColumnStretch(i, 1);

    QString borderStyle = (theme == "1bit") ? "border: none;" : "border: 1px solid #555;";
    QString borderRadius = (theme == "1bit") ? "border-radius: 0px;" : "border-radius: 2px;";

    for (int i = 0; i < m_palette.size(); ++i) {
        QPushButton *colorBtn = new QPushButton();
        colorBtn->setFixedHeight(24);
        colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        colorBtn->setCursor(Qt::PointingHandCursor);

        colorBtn->setStyleSheet(QString("background-color: %1; %2 %3").arg(m_palette[i].name(), borderStyle, borderRadius));
        colorBtn->setProperty("swatchColor", m_palette[i]);
        colorBtn->setProperty("colorIndex", i);
        colorBtn->installEventFilter(this);

        m_gridLayout->addWidget(colorBtn, i / columns, i % columns);
    }

    QPushButton *btnAddColor = new QPushButton("+");
    btnAddColor->setFixedHeight(24);
    btnAddColor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnAddColor->setCursor(Qt::PointingHandCursor);

    if (theme == "1bit") {
        btnAddColor->setStyleSheet("background-color: transparent; border: 1px dashed white; color: white; font-size: 16px; font-weight: bold;");
    } else {
        btnAddColor->setStyleSheet("background-color: transparent; border: 1px dashed #888888; color: #aaaaaa; font-size: 16px; font-weight: bold; border-radius: 2px;");
    }

    connect(btnAddColor, &QPushButton::clicked, this, &PaletteWidget::onAddNewColorClicked);

    int nextIndex = m_palette.size();
    m_gridLayout->addWidget(btnAddColor, nextIndex / columns, nextIndex % columns);

    savePaletteToSettings();
}

void PaletteWidget::onAddNewColorClicked() {
    emit requestEditColor(-1);
}

bool PaletteWidget::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        QPushButton *btn = qobject_cast<QPushButton*>(watched);
        if (btn && btn->property("swatchColor").isValid()) {
            int colorIndex = btn->property("colorIndex").toInt();

            if (event->type() == QEvent::MouseButtonDblClick) {
                emit requestEditColor(colorIndex);
                return true;
            } else if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                QColor clickedColor = btn->property("swatchColor").value<QColor>();

                if (me->button() == Qt::LeftButton && (me->modifiers() & Qt::ShiftModifier)) {
                    if (m_mainSwapIndex == -1) {
                        m_mainSwapIndex = colorIndex;
                        btn->setStyleSheet(btn->styleSheet() + " border: 2px dashed white;");
                    } else {
                        if (m_mainSwapIndex < m_palette.size() && colorIndex < m_palette.size()) {
                            m_palette.swapItemsAt(m_mainSwapIndex, colorIndex);
                            emit paletteChanged();
                        }
                        m_mainSwapIndex = -1;
                        rebuildGrid();
                    }
                    return true;
                } else {
                    m_mainSwapIndex = -1;
                }

                if (me->button() == Qt::MiddleButton) {
                    m_palette.removeAt(colorIndex);
                    emit paletteChanged();
                    rebuildGrid();
                    return true;
                } else if (me->button() == Qt::LeftButton) {
                    emit primaryColorSelected(clickedColor);
                } else if (me->button() == Qt::RightButton) {
                    emit secondaryColorSelected(clickedColor);
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
