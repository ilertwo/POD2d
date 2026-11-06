#include "layerlistwidget.h"
#include "projectmodel.h"
#include <QSettings>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>

LayerListWidget::LayerListWidget(QWidget *parent) : QListWidget(parent) {
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setViewMode(QListView::ListMode);
    this->setIconSize(QSize(64, 32));
    this->setSpacing(3);
    this->setDragDropMode(QAbstractItemView::InternalMove);
    this->setResizeMode(QListView::Adjust);

    setHorizontal(false);

    connect(this, &QWidget::customContextMenuRequested, this, &LayerListWidget::showContextMenu);
    connect(this, &QListWidget::currentRowChanged, this, &LayerListWidget::onCurrentRowChanged);

    connect(this->model(), &QAbstractItemModel::rowsMoved, this, &LayerListWidget::onRowsMoved);

    this->viewport()->installEventFilter(this);
}

void LayerListWidget::setHorizontal(bool horizontal) {
    m_horizontal = horizontal;

    setFlow(horizontal ? QListView::LeftToRight : QListView::TopToBottom);
    setWrapping(false);
    setVerticalScrollBarPolicy  (horizontal ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(horizontal ? Qt::ScrollBarAsNeeded  : Qt::ScrollBarAlwaysOff);
    setMinimumHeight(horizontal ? 70 : 0);
    setMinimumWidth (horizontal ? 0  : 120);

    updateItemSizes();
}

void LayerListWidget::updateItemSizes() {
    QSize hint = kLayerItem;
    if (!m_horizontal)
        hint.setWidth(qMax(80, viewport()->width() - 2 * spacing() - 1));
    for (int i = 0; i < count(); ++i)
        item(i)->setSizeHint(hint);
}

void LayerListWidget::resizeEvent(QResizeEvent *event) {
    QListWidget::resizeEvent(event);
    if (!m_horizontal) updateItemSizes();
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

    const QPixmap pix = QPixmap::fromImage(m_model->getLayerThumbnail(index))
                            .scaled(kLayerThumb, Qt::KeepAspectRatio, Qt::FastTransformation);
    imgLabel->setPixmap(pix);
    imgLabel->setFixedSize(pix.size());
}

void LayerListWidget::onRowsMoved(const QModelIndex &, int start, int, const QModelIndex &, int row) {
    if (!m_model) return;
    int toIndex = (row > start) ? row - 1 : row;
    m_model->moveLayer(start, toIndex);
}

void LayerListWidget::rebuildList() {
    if (!m_model) return;
    const QSignalBlocker blocker(this);
    setUpdatesEnabled(false);
    clear();

    const int layerCount = m_model->getLayerCount();

    for (int i = 0; i < layerCount; ++i) {
        auto *item = new QListWidgetItem();
        item->setSizeHint(kLayerItem);
        addItem(item);

        auto *cell = new QWidget();
        cell->setObjectName("layerCell");

        auto *layout = new QHBoxLayout(cell);
        layout->setContentsMargins(6, 4, 8, 4);
        layout->setSpacing(6);

        auto *imageLabel = new QLabel();
        imageLabel->setObjectName("layerImage");
        const QPixmap pix = QPixmap::fromImage(m_model->getLayerThumbnail(i))
                                .scaled(kLayerThumb, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pix);
        imageLabel->setFixedSize(pix.size());

        auto *textLabel = new QLabel(QString::number(i));
        textLabel->setObjectName("layerNumber");
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setProperty("hiddenItem", !m_model->isLayerVisible(i));

        layout->addWidget(imageLabel, 0, Qt::AlignVCenter);
        layout->addStretch();
        layout->addWidget(textLabel, 0, Qt::AlignVCenter);

        setItemWidget(item, cell);
    }

    setUpdatesEnabled(true);
    updateItemSizes();
    setCurrentRow(m_model->getCurrentLayerIndex());
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
