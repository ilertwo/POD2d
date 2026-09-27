#include "framelistwidget.h"
#include "projectmodel.h"
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QMouseEvent>
#include <QEvent>

FrameListWidget::FrameListWidget(QWidget *parent) : QListWidget(parent) {
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setDragDropMode(QAbstractItemView::InternalMove);
    this->setWrapping(false);
    this->setSpacing(6);
    this->setFixedHeight(125);
    this->setViewMode(QListView::IconMode);
    this->setMovement(QListView::Static);

    this->setFlow(QListView::LeftToRight);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(this, &QWidget::customContextMenuRequested, this, &FrameListWidget::showContextMenu);
    connect(this, &QListWidget::currentRowChanged, this, &FrameListWidget::onCurrentRowChanged);
    connect(this->model(), &QAbstractItemModel::rowsMoved, this, &FrameListWidget::onRowsMoved);

    this->viewport()->installEventFilter(this);
}

void FrameListWidget::setModel(ProjectModel *model) {
    m_model = model;

    connect(m_model, &ProjectModel::framesListChanged, this, &FrameListWidget::rebuildList);
    connect(m_model, &ProjectModel::frameChanged, this, &FrameListWidget::onActiveFrameChanged);

    connect(m_model, &ProjectModel::forceUIFrameSelection, this, [this](int index) {
        this->blockSignals(true);
        this->setCurrentRow(index);
        this->blockSignals(false);
    });

    connect(m_model, &ProjectModel::imageChanged, this, [this](const QImage &flatImg) {
        int currentIndex = m_model->getCurrentFrameIndex();
        QListWidgetItem *item = this->item(currentIndex);
        if (item) {
            if (QWidget *cellWidget = this->itemWidget(item)) {
                if (QLabel *imgLabel = cellWidget->findChild<QLabel*>("frameImage")) {
                    QPixmap newPix = QPixmap::fromImage(flatImg).scaled(128, 64, Qt::KeepAspectRatio, Qt::FastTransformation);
                    imgLabel->setPixmap(newPix);
                }
            }
        }
    });

    rebuildList();
}

void FrameListWidget::reloadTheme() {
    rebuildList();
}

void FrameListWidget::onCurrentRowChanged(int row) {
    if (m_model && row >= 0) {
        m_model->setCurrentFrame(row);
    }
}

void FrameListWidget::onActiveFrameChanged(int index) {
    this->blockSignals(true);
    if (index >= 0 && index < this->count()) {
        this->setCurrentRow(index);
    }
    this->blockSignals(false);
}

void FrameListWidget::onRowsMoved(const QModelIndex &, int start, int, const QModelIndex &, int row) {
    if (!m_model) return;
    int toIndex = (row > start) ? row - 1 : row;
    if (start == toIndex) {
        rebuildList();
    } else {
        m_model->moveFrame(start, toIndex);
    }
}

bool FrameListWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == this->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (!this->itemAt(me->pos()) && m_model) {
            m_model->duplicateCurrentFrame();
            return true;
        }
    }
    return QListWidget::eventFilter(watched, event);
}

void FrameListWidget::rebuildList() {
    if (!m_model) return;
    const QSignalBlocker blocker(this);
    this->clear();

    const int frameCount = m_model->getFrameCount();
    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

    QString topBorder, normalItemBorder, selectedItemBorder, itemBg, textColor;
    int borderRadius = (theme == "1bit") ? 0 : 6;

    if (theme == "1bit") {
        topBorder = "border-top: 2px solid white;";
        normalItemBorder = "border: 1px solid white;";
        selectedItemBorder = "border: 2px solid white;";
        itemBg = "black";
        textColor = "white";
    } else if (theme == "light") {
        topBorder = "border-top: 1px solid #d4d4d4;";
        normalItemBorder = "border: 1px solid #d4d4d4;";
        selectedItemBorder = "border: 2px solid #0078d7;";
        itemBg = "#e0e0e0";
        textColor = "#202020";
    } else { // dark
        topBorder = "border-top: 2px solid #333333;";
        normalItemBorder = "border: 1px solid #444444;";
        selectedItemBorder = "border: 2px solid #888888;";
        itemBg = "#333333";
        textColor = "white";
    }

    this->setStyleSheet(
        "QListWidget { outline: 0; background: transparent; border: none; " + topBorder + " padding-top: 4px; }"
                                                                                          "QListWidget::item { background-color: " + itemBg + "; " + normalItemBorder + " border-radius: " + QString::number(borderRadius) + "px; }"
                                                                                                  "QListWidget::item:selected { " + selectedItemBorder + " }"
        );

    for (int i = 0; i < frameCount; ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(140, 88));
        this->addItem(item);

        QFrame *frameWidget = new QFrame();
        frameWidget->setStyleSheet("background: transparent; border: none;");

        QVBoxLayout *layout = new QVBoxLayout(frameWidget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(2);
        layout->setAlignment(Qt::AlignCenter);

        QLabel *imageLabel = new QLabel();
        imageLabel->setObjectName("frameImage");
        imageLabel->setFixedSize(128, 64);
        imageLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0.2); border-radius: 2px;");
        imageLabel->setAlignment(Qt::AlignCenter);

        QImage thumb = m_model->getFrameThumbnail(i);
        QPixmap pixmap = QPixmap::fromImage(thumb).scaled(128, 64, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pixmap);

        QLabel *textLabel = new QLabel(QString::number(i + 1));
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setFixedHeight(16);

        bool isVis = m_model->isFrameVisible(i);
        if (isVis) {
            textLabel->setStyleSheet("color: " + textColor + "; font-size: 11px; font-weight: bold; background: transparent;");
        } else {
            textLabel->setStyleSheet("color: #777777; font-size: 11px; font-weight: bold; text-decoration: line-through; background: transparent;");
        }

        layout->addWidget(imageLabel);
        layout->addWidget(textLabel);

        this->setItemWidget(item, frameWidget);
    }

    this->setCurrentRow(m_model->getCurrentFrameIndex());
}

void FrameListWidget::showContextMenu(const QPoint &pos) {
    if (!m_model) return;
    QListWidgetItem *item = this->itemAt(pos);
    if (!item) return;

    int frameIndex = this->row(item);
    QMenu contextMenu(this);

    bool isVisible = m_model->isFrameVisible(frameIndex);
    QAction *actToggleVisibility = contextMenu.addAction(isVisible ? "Hide frame" : "Show frame");
    QAction *actDuplicate = contextMenu.addAction("Duplicate frame");
    QAction *actMoveLeft = contextMenu.addAction("Move left");
    actMoveLeft->setEnabled(frameIndex > 0);
    QAction *actMoveRight = contextMenu.addAction("Move to the right");
    actMoveRight->setEnabled(frameIndex < m_model->getFrameCount() - 1);
    QAction *actDelete = contextMenu.addAction("Delete frame");
    actDelete->setEnabled(m_model->getFrameCount() > 1);

    QAction *selectedAction = contextMenu.exec(this->mapToGlobal(pos));

    if (selectedAction == actToggleVisibility) m_model->toggleFrameVisibility(frameIndex);
    else if (selectedAction == actDuplicate) { m_model->setCurrentFrame(frameIndex); m_model->duplicateCurrentFrame(); }
    else if (selectedAction == actMoveLeft) m_model->moveFrame(frameIndex, frameIndex - 1);
    else if (selectedAction == actMoveRight) m_model->moveFrame(frameIndex, frameIndex + 1);
    else if (selectedAction == actDelete) { m_model->setCurrentFrame(frameIndex); m_model->deleteCurrentFrame(); }
}
