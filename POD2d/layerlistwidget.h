#ifndef LAYERLISTWIDGET_H
#define LAYERLISTWIDGET_H

#include <QListWidget>
#include <QMenu>
#include <QResizeEvent>

class ProjectModel;

class LayerListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit LayerListWidget(QWidget *parent = nullptr);

    void setModel(ProjectModel *model);
    void reloadTheme();
    void rebuildList();
    void setHorizontal(bool horizontal);
    bool m_horizontal = false;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showContextMenu(const QPoint &pos);
    void onActiveLayerChanged(int index);
    void onThumbnailUpdated(int index);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onCurrentRowChanged(int row);
    void updateItemSizes();

private:
    ProjectModel *m_model = nullptr;
};

#endif // LAYERLISTWIDGET_H
