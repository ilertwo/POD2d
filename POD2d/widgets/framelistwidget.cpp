#include "framelistwidget.h"
#include "projectmodel.h"
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEvent>

FrameListWidget::FrameListWidget(QWidget *parent) : QListWidget(parent) {
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setDragDropMode(QAbstractItemView::InternalMove);
    this->setWrapping(false);
    this->setSpacing(6);
    this->setViewMode(QListView::IconMode);
    this->setMovement(QListView::Static);
    this->setResizeMode(QListView::Adjust);

    setHorizontal(true);

    connect(this, &QWidget::customContextMenuRequested, this, &FrameListWidget::showContextMenu);
    connect(this, &QListWidget::currentRowChanged, this, &FrameListWidget::onCurrentRowChanged);
    connect(this->model(), &QAbstractItemModel::rowsMoved, this, &FrameListWidget::onRowsMoved);

    this->viewport()->installEventFilter(this);
}

void FrameListWidget::setHorizontal(bool horizontal) {
    m_horizontal = horizontal;

    setFlow(horizontal ? QListView::LeftToRight : QListView::TopToBottom);
    setWrapping(false);
    setVerticalScrollBarPolicy  (horizontal ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(horizontal ? Qt::ScrollBarAsNeeded  : Qt::ScrollBarAlwaysOff);
    setMinimumHeight(horizontal ? kFrameItem.height() + 22 : 0);
    setMinimumWidth (horizontal ? 0 : 150);

    updateItemSizes();
}

void FrameListWidget::updateItemSizes() {
    QSize hint = kFrameItem;
    if (!m_horizontal)
        hint.setWidth(qMax(kFrameItem.width(), viewport()->width() - 2 * spacing() - 1));
    for (int i = 0; i < count(); ++i)
        item(i)->setSizeHint(hint);
}

void FrameListWidget::resizeEvent(QResizeEvent *event) {
    QListWidget::resizeEvent(event);
    if (!m_horizontal) updateItemSizes();
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
        QListWidgetItem *it = item(m_model->getCurrentFrameIndex());
        if (!it) return;
        QWidget *cell = itemWidget(it);
        auto *imgLabel = cell ? cell->findChild<QLabel*>("frameImage") : nullptr;
        if (!imgLabel) return;
            const QPixmap pix = QPixmap::fromImage(m_model->applyBackground(flatImg)).scaled(kFrameThumb, Qt::KeepAspectRatio, Qt::FastTransformation);
            imgLabel->setPixmap(pix);
            imgLabel->setFixedSize(pix.size());
    });

    rebuildList();
}

void FrameListWidget::reloadTheme() {
    viewport()->update();
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
    setUpdatesEnabled(false);
    clear();

    const int frameCount = m_model->getFrameCount();

    for (int i = 0; i < frameCount; ++i) {
        auto *item = new QListWidgetItem();
        item->setSizeHint(kFrameItem);
        addItem(item);

        auto *cell = new QFrame();
        cell->setObjectName("frameCell");

        auto *layout = new QVBoxLayout(cell);
        layout->setContentsMargins(3, 3, 3, 3);
        layout->setSpacing(2);

        auto *imageLabel = new QLabel();
        imageLabel->setObjectName("frameImage");
        const QPixmap pix = QPixmap::fromImage(m_model->getFrameThumbnail(i))
                                .scaled(kFrameThumb, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pix);
        imageLabel->setFixedSize(pix.size());

        auto *textLabel = new QLabel(QString::number(i + 1));
        textLabel->setObjectName("frameNumber");
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setFixedHeight(16);
        textLabel->setProperty("hiddenItem", !m_model->isFrameVisible(i));

        layout->addWidget(imageLabel, 0, Qt::AlignHCenter);
        layout->addWidget(textLabel);

        setItemWidget(item, cell);
    }

    setUpdatesEnabled(true);
    updateItemSizes();
    setCurrentRow(m_model->getCurrentFrameIndex());
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
