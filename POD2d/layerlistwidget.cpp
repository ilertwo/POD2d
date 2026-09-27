#include "layerlistwidget.h"
#include "projectmodel.h"
#include <QSettings>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QEvent>
#include <QMouseEvent>

LayerListWidget::LayerListWidget(QWidget *parent) : QListWidget(parent) {
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setViewMode(QListView::ListMode);
    this->setIconSize(QSize(64, 32));
    this->setSpacing(3);
    this->setDragDropMode(QAbstractItemView::InternalMove);

    connect(this, &QWidget::customContextMenuRequested, this, &LayerListWidget::showContextMenu);
    connect(this, &QListWidget::currentRowChanged, this, &LayerListWidget::onCurrentRowChanged);

    connect(this->model(), &QAbstractItemModel::rowsMoved, this, &LayerListWidget::onRowsMoved);

    this->viewport()->installEventFilter(this);
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
    rebuildList();
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
    if (m_model && index >= 0 && index < this->count()) {
        QImage thumb = m_model->getLayerThumbnail(index);
        QListWidgetItem *item = this->item(index);

        if (QWidget *cellWidget = this->itemWidget(item)) {
            if (QLabel *imgLabel = cellWidget->findChild<QLabel*>("layerImage")) {
                QPixmap newPix = QPixmap::fromImage(thumb).scaled(100, 32, Qt::KeepAspectRatio, Qt::FastTransformation);
                imgLabel->setPixmap(newPix);
            }
        }
    }
}

void LayerListWidget::onRowsMoved(const QModelIndex &, int start, int, const QModelIndex &, int row) {
    if (!m_model) return;
    int toIndex = (row > start) ? row - 1 : row;
    m_model->moveLayer(start, toIndex);
}

void LayerListWidget::rebuildList() {
    if (!m_model) return;
    const QSignalBlocker blocker(this);
    this->clear();

    const int count = m_model->getLayerCount();
    QSettings settings("POD2d", "EditorSettings");
    QString theme = settings.value("ui/theme", "dark").toString();

    QString normalItemBorder, selectedItemBorder, itemBg, textColor;
    int borderRadius = (theme == "1bit") ? 0 : 6;

    if (theme == "1bit") {
        normalItemBorder = "border: 1px solid white;";
        selectedItemBorder = "border: 2px solid white;";
        itemBg = "black";
        textColor = "white";
    } else if (theme == "light") {
        normalItemBorder = "border: 1px solid #d4d4d4;";
        selectedItemBorder = "border: 2px solid #0078d7;";
        itemBg = "#e0e0e0";
        textColor = "#202020";
    } else { // dark
        normalItemBorder = "border: 1px solid #444444;";
        selectedItemBorder = "border: 2px solid #888888;";
        itemBg = "#333333";
        textColor = "white";
    }

    this->setStyleSheet(
        "QListWidget { outline: 0; background: transparent; border: none; }"
        "QListWidget::item { background-color: " + itemBg + "; " + normalItemBorder + " border-radius: " + QString::number(borderRadius) + "px; margin: 2px; }"
                                                                                                  "QListWidget::item:selected { " + selectedItemBorder + " }"
        );

    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setSizeHint(QSize(110, 46));
        this->addItem(item);

        QWidget *rowWidget = new QWidget();
        rowWidget->setStyleSheet("background: transparent; border: none;");

        QHBoxLayout *layout = new QHBoxLayout(rowWidget);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(5);

        QLabel *imageLabel = new QLabel();
        imageLabel->setObjectName("layerImage");
        imageLabel->setFixedSize(80, 30);
        imageLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0.2); border-radius: 2px;");
        imageLabel->setAlignment(Qt::AlignCenter);

        QImage thumb = m_model->getLayerThumbnail(i);
        QPixmap pixmap = QPixmap::fromImage(thumb).scaled(80, 30, Qt::KeepAspectRatio, Qt::FastTransformation);
        imageLabel->setPixmap(pixmap);

        QLabel *textLabel = new QLabel(QString::number(i));
        textLabel->setAlignment(Qt::AlignCenter);

        bool isVis = m_model->isLayerVisible(i);
        if (isVis) {
            textLabel->setStyleSheet("color: " + textColor + "; font-weight: bold; background: transparent;");
        } else {
            textLabel->setStyleSheet("color: #777777; font-weight: bold; text-decoration: line-through; background: transparent;");
        }

        layout->addWidget(imageLabel);
        layout->addStretch();
        layout->addWidget(textLabel);

        this->setItemWidget(item, rowWidget);
    }

    this->setCurrentRow(m_model->getCurrentLayerIndex());
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
