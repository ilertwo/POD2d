#include "layerlistwidget.h"
#include "projectmodel.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QImage>
#include <QStyledItemDelegate>
#include <QTimer>

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

LayerListWidget::LayerListWidget(QWidget *parent) : QListWidget(parent) {
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setViewMode(QListView::ListMode);
    this->setIconSize(QSize(64, 32));
    this->setSpacing(3);
    this->setDragDropMode(QAbstractItemView::NoDragDrop);
    this->setResizeMode(QListView::Adjust);
    setItemDelegate(new FullRectDelegate(this));

    setHorizontal(false);

    connect(this, &QWidget::customContextMenuRequested, this, &LayerListWidget::showContextMenu);
    connect(this, &QListWidget::currentRowChanged, this, &LayerListWidget::onCurrentRowChanged);

    this->viewport()->installEventFilter(this);
}

void LayerListWidget::setHorizontal(bool horizontal) {
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
    setMinimumHeight(horizontal ? 96 : 0);
    setMinimumWidth (horizontal ? 0  : 120);

    if (changed && count() > 0) {
        rebuildList();
        return;
    }

    setProperty("cellSize", QSize());
    setProperty("cellBox", QSize());
    updateItemSizes();
    QTimer::singleShot(0, this, [this]() { reloadTheme(); });
}

void LayerListWidget::updateItemSizes() {
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
        auto *imgLabel = cellWidget ? cellWidget->findChild<QLabel*>("layerImage") : nullptr;
        if (!imgLabel) continue;

        QImage src = it->data(Qt::UserRole + 1).value<QImage>();
        if (src.isNull() && m_model) {
            src = m_model->getLayerThumbnail(i);
            it->setData(Qt::UserRole + 1, src);
        }
        if (src.isNull()) continue;

        const QPixmap pix = QPixmap::fromImage(src).scaled(box, Qt::KeepAspectRatio, Qt::FastTransformation);
        imgLabel->setPixmap(pix);
        imgLabel->setFixedSize(pix.size());
    }
}

void LayerListWidget::resizeEvent(QResizeEvent *event) {
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

void LayerListWidget::setModel(ProjectModel *model) {
    m_model = model;

    connect(m_model, &ProjectModel::layersListChanged, this, &LayerListWidget::rebuildList);
    connect(m_model, &ProjectModel::activeLayerChanged, this, &LayerListWidget::onActiveLayerChanged);
    connect(m_model, &ProjectModel::layerThumbnailUpdated, this, &LayerListWidget::onThumbnailUpdated);
    connect(m_model, &ProjectModel::forceUILayerSelection, this, [this](int index) {
        this->blockSignals(true);
        this->setCurrentRow(index);
        this->blockSignals(false);
    });

    rebuildList();
}

void LayerListWidget::reloadTheme() {
    updateItemSizes();
    doItemsLayout();
    viewport()->update();
}

void LayerListWidget::onCurrentRowChanged(int row) {
    if (m_model && row >= 0) {
        m_model->setCurrentLayer(row);
    }
}

void LayerListWidget::onActiveLayerChanged(int index) {
    this->blockSignals(true);
    if (index >= 0 && index < this->count()) {
        this->setCurrentRow(index);
    }
    this->blockSignals(false);
}

void LayerListWidget::onThumbnailUpdated(int index) {
    if (!m_model || index < 0 || index >= count()) return;

    QWidget *cell = itemWidget(item(index));
    auto *imgLabel = cell ? cell->findChild<QLabel*>("layerImage") : nullptr;
    if (!imgLabel) return;

    const QImage thumb = m_model->getLayerThumbnail(index);
    item(index)->setData(Qt::UserRole + 1, thumb);

    QSize box = property("cellBox").toSize();
    if (!box.isValid()) box = kLayerThumb;

    const QPixmap pix = QPixmap::fromImage(thumb).scaled(box, Qt::KeepAspectRatio, Qt::FastTransformation);
    imgLabel->setPixmap(pix);
    imgLabel->setFixedSize(pix.size());
}

void LayerListWidget::onRowsMoved(const QModelIndex &, int start, int, const QModelIndex &, int row) {
    if (!m_model) return;
    int toIndex = (row > start) ? row - 1 : row;
    QTimer::singleShot(0, this, [this, start, toIndex]() {
        if (m_model) m_model->moveLayer(start, toIndex);
    });
}

bool LayerListWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == this->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
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
                            m_model->moveLayer(from, targetRow);
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

void LayerListWidget::rebuildList() {
    if (!m_model) return;
    const QSignalBlocker blocker(this);
    setUpdatesEnabled(false);
    clear();

    const int layerCount = m_model->getLayerCount();

    for (int i = 0; i < layerCount; ++i) {
        auto *item = new QListWidgetItem();
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        item->setSizeHint(kLayerItem);
        addItem(item);

        auto *cell = new QWidget();
        cell->setObjectName("layerCell");
        cell->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto *imageLabel = new QLabel();
        imageLabel->setObjectName("layerImage");
        const QImage thumb = m_model->getLayerThumbnail(i);
        item->setData(Qt::UserRole + 1, thumb);
        const QPixmap pix = QPixmap::fromImage(thumb)
                                .scaled(kLayerThumb, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pix);
        imageLabel->setFixedSize(pix.size());

        auto *textLabel = new QLabel(QString::number(i));
        textLabel->setObjectName("layerNumber");
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setProperty("hiddenItem", !m_model->isLayerVisible(i));

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
    setCurrentRow(m_model->getCurrentLayerIndex());
    QTimer::singleShot(0, this, [this]() { reloadTheme(); });
}

void LayerListWidget::showContextMenu(const QPoint &pos) {
    if (!m_model) return;
    QListWidgetItem *item = this->itemAt(pos);
    if (!item) return;

    int layerIndex = this->row(item);
    QMenu contextMenu(this);

    bool isVisible = m_model->isLayerVisible(layerIndex);
    QAction *actToggleVisibility = contextMenu.addAction(isVisible ? "Hide layer" : "Show layer");
    actToggleVisibility->setShortcut(QKeySequence("Ctrl+H"));

    QAction *actDuplicate = contextMenu.addAction("Duplicate layer");

    QAction *actMergeDown = contextMenu.addAction("Merge from below");
    actMergeDown->setEnabled(layerIndex > 0);

    QAction *actMoveUp = contextMenu.addAction("Move up");
    actMoveUp->setShortcut(QKeySequence("Ctrl+Shift+PgUp"));
    actMoveUp->setEnabled(layerIndex < m_model->getLayerCount() - 1);

    QAction *actMoveDown = contextMenu.addAction("Move down");
    actMoveDown->setShortcut(QKeySequence("Ctrl+Shift+PgDown"));
    actMoveDown->setEnabled(layerIndex > 0);

    QAction *actDelete = contextMenu.addAction("Delete layer");
    actDelete->setShortcut(QKeySequence("Ctrl+Shift+Del"));
    actDelete->setEnabled(m_model->getLayerCount() > 1);

    QAction *selectedAction = contextMenu.exec(this->mapToGlobal(pos));

    if (selectedAction == actToggleVisibility) m_model->toggleLayerVisibility(layerIndex);
    else if (selectedAction == actDuplicate) m_model->duplicateLayer(layerIndex);
    else if (selectedAction == actMergeDown) { m_model->setCurrentLayer(layerIndex); m_model->mergeLayerDown(); }
    else if (selectedAction == actMoveUp) m_model->moveLayer(layerIndex, layerIndex + 1);
    else if (selectedAction == actMoveDown) m_model->moveLayer(layerIndex, layerIndex - 1);
    else if (selectedAction == actDelete) { m_model->setCurrentLayer(layerIndex); m_model->deleteCurrentLayer(); }
}
