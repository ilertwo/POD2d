#include "framelistwidget.h"
#include "projectmodel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QImage>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QEvent>

namespace {
class FullRectDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &) const override {
        editor->setGeometry(option.rect);
    }
};
}

FrameListWidget::FrameListWidget(QWidget *parent) : QListWidget(parent) {
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setDragDropMode(QAbstractItemView::InternalMove);
    this->setWrapping(false);
    this->setSpacing(6);
    this->setViewMode(QListView::IconMode);
    this->setMovement(QListView::Static);
    this->setResizeMode(QListView::Adjust);
    setItemDelegate(new FullRectDelegate(this));

    setHorizontal(true);

    connect(this, &QWidget::customContextMenuRequested, this, &FrameListWidget::showContextMenu);
    connect(this, &QListWidget::currentRowChanged, this, &FrameListWidget::onCurrentRowChanged);
    connect(this->model(), &QAbstractItemModel::rowsMoved, this, &FrameListWidget::onRowsMoved);

    this->viewport()->installEventFilter(this);
}

void FrameListWidget::setHorizontal(bool horizontal) {
    const bool changed = (m_horizontal != horizontal);
    m_horizontal = horizontal;

    setViewMode(horizontal ? QListView::IconMode : QListView::ListMode);
    setMovement(QListView::Static);
    setDragEnabled(false);
    setAcceptDrops(false);
    setDragDropMode(QAbstractItemView::NoDragDrop);

    setSpacing(horizontal ? 6 : 3);
    setFlow(horizontal ? QListView::LeftToRight : QListView::TopToBottom);
    setWrapping(false);
    setVerticalScrollBarPolicy  (horizontal ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(horizontal ? Qt::ScrollBarAsNeeded  : Qt::ScrollBarAlwaysOff);
    setMinimumHeight(horizontal ? kFrameItem.height() + 22 : 0);
    setMinimumWidth (horizontal ? 0 : 120);

    if (changed && count() > 0) {
        rebuildList();
        return;
    }

    setProperty("cellSize", QSize());
    setProperty("cellBox", QSize());
    updateItemSizes();
    QTimer::singleShot(0, this, [this]() { reloadTheme(); });
}

void FrameListWidget::updateItemSizes() {
    QSize cell;
    QSize box;

    if (m_horizontal) {
        const QScrollBar *bar = horizontalScrollBar();
        const int fullH = viewport()->height() + (bar->isVisible() ? bar->height() : 0);
        const int h = qBound(88, (fullH - bar->sizeHint().height() - 2 * spacing() - 2) / 4 * 4, 120);
        const int w = qMax(96, int((h - 22) * 1.8));
        cell = QSize(w, h);
        box  = QSize(cell.width() - 8, cell.height() - 24);
    } else {
        cell = QSize(qMax(80, viewport()->width() - 2 * spacing() - 1), 48);
        box  = QSize(72, 40);
    }

    const bool sizeChanged = property("cellSize").toSize() != cell;
    const bool boxChanged  = property("cellBox").toSize()  != box;
    if (!sizeChanged && !boxChanged) return;
    setProperty("cellSize", cell);
    setProperty("cellBox", box);

    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *it = item(i);
        it->setSizeHint(cell);
        if (!boxChanged) continue;

        QWidget *cellWidget = itemWidget(it);
        auto *imgLabel = cellWidget ? cellWidget->findChild<QLabel*>("frameImage") : nullptr;
        if (!imgLabel) continue;

        QImage src = it->data(Qt::UserRole + 1).value<QImage>();
        if (src.isNull() && m_model) {
            src = m_model->getFrameThumbnail(i);
            it->setData(Qt::UserRole + 1, src);
        }
        if (src.isNull()) continue;

        const QPixmap pix = QPixmap::fromImage(src).scaled(box, Qt::KeepAspectRatio, Qt::FastTransformation);
        imgLabel->setPixmap(pix);
        imgLabel->setFixedSize(pix.size());
    }
}

void FrameListWidget::resizeEvent(QResizeEvent *event) {
    QListWidget::resizeEvent(event);
    updateItemSizes();
    if (!property("_relayoutPending").toBool()) {
        setProperty("_relayoutPending", true);
        QTimer::singleShot(0, this, [this]() {
            setProperty("_relayoutPending", false);
            doItemsLayout();
        });
    }
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

        const QImage bg = m_model->applyBackground(flatImg);
        it->setData(Qt::UserRole + 1, bg);

        QSize box = property("cellBox").toSize();
        if (!box.isValid()) box = kFrameThumb;

        const QPixmap pix = QPixmap::fromImage(bg).scaled(box, Qt::KeepAspectRatio, Qt::FastTransformation);
        imgLabel->setPixmap(pix);
        imgLabel->setFixedSize(pix.size());
    });

    rebuildList();
}

void FrameListWidget::reloadTheme() {
    updateItemSizes();
    doItemsLayout();
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
    QTimer::singleShot(0, this, [this, start, toIndex]() {
        if (!m_model) return;
        if (start == toIndex) {
            rebuildList();
        } else {
            m_model->moveFrame(start, toIndex);
        }
    });
}

bool FrameListWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == this->viewport()) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (!this->itemAt(me->pos()) && m_model) {
                m_model->duplicateCurrentFrame();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                QListWidgetItem *it = itemAt(me->pos());
                if (it) {
                    m_dragStartRow = row(it);
                    m_dragStartPos = me->pos();
                    m_isDraggingItem = false;
                }
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent*>(event);
            if ((me->buttons() & Qt::LeftButton) && m_dragStartRow >= 0 && m_model) {
                if (!m_isDraggingItem) {
                    if ((me->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
                        m_isDraggingItem = true;
                        setCursor(Qt::ClosedHandCursor);
                    }
                }
                if (m_isDraggingItem) {
                    QPoint probe = me->pos();
                    if (m_horizontal) probe.setY(viewport()->height() / 2);
                    else              probe.setX(viewport()->width() / 2);

                    QListWidgetItem *target = itemAt(probe);
                    if (target) {
                        int targetRow = row(target);
                        if (targetRow >= 0 && targetRow != m_dragStartRow) {
                            int from = m_dragStartRow;
                            m_dragStartRow = targetRow;
                            m_model->moveFrame(from, targetRow);
                        }
                    }
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_dragStartRow >= 0) {
                m_dragStartRow = -1;
                if (m_isDraggingItem) {
                    m_isDraggingItem = false;
                    unsetCursor();
                    return true;
                }
            }
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
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        item->setSizeHint(kFrameItem);
        addItem(item);

        auto *cell = new QFrame();
        cell->setObjectName("frameCell");
        cell->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto *imageLabel = new QLabel();
        imageLabel->setObjectName("frameImage");
        const QImage thumb = m_model->getFrameThumbnail(i);
        item->setData(Qt::UserRole + 1, thumb);
        const QSize initBox = m_horizontal ? kFrameThumb : QSize(72, 40);
        const QPixmap pix = QPixmap::fromImage(thumb)
                                .scaled(initBox, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pix);
        imageLabel->setFixedSize(pix.size());

        auto *textLabel = new QLabel(QString::number(i + 1));
        textLabel->setObjectName("frameNumber");
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setProperty("hiddenItem", !m_model->isFrameVisible(i));

        if (m_horizontal) {
            auto *layout = new QVBoxLayout(cell);
            layout->setContentsMargins(3, 3, 3, 3);
            layout->setSpacing(2);
            textLabel->setFixedHeight(16);
            layout->addWidget(imageLabel, 0, Qt::AlignHCenter);
            layout->addWidget(textLabel);
        } else {
            auto *layout = new QHBoxLayout(cell);
            layout->setContentsMargins(6, 4, 8, 4);
            layout->setSpacing(6);
            layout->addWidget(imageLabel, 0, Qt::AlignVCenter);
            layout->addStretch();
            layout->addWidget(textLabel, 0, Qt::AlignVCenter);
        }

        setItemWidget(item, cell);
    }

    setUpdatesEnabled(true);
    setProperty("cellSize", QSize());
    setProperty("cellBox", QSize());
    updateItemSizes();
    setCurrentRow(m_model->getCurrentFrameIndex());
    QTimer::singleShot(0, this, [this]() { reloadTheme(); });
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
