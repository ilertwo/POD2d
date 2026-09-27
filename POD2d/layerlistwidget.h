#ifndef LAYERLISTWIDGET_H
#define LAYERLISTWIDGET_H

#include <QListWidget>
#include <QMenu>

class ProjectModel;

class LayerListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit LayerListWidget(QWidget *parent = nullptr);

    void setModel(ProjectModel *model);
    void reloadTheme();
    void rebuildList();

private slots:
    void showContextMenu(const QPoint &pos);
    void onActiveLayerChanged(int index);
    void onThumbnailUpdated(int index);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onCurrentRowChanged(int row);

private:
    ProjectModel *m_model = nullptr;
};

#endif // LAYERLISTWIDGET_H
