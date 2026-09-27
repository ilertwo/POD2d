#ifndef FRAMELISTWIDGET_H
#define FRAMELISTWIDGET_H

#include <QListWidget>
#include <QMenu>

class ProjectModel;

class FrameListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit FrameListWidget(QWidget *parent = nullptr);

    void setModel(ProjectModel *model);

public slots:
    void rebuildList();
    void reloadTheme();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void showContextMenu(const QPoint &pos);
    void onActiveFrameChanged(int index);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onCurrentRowChanged(int row);

private:
    ProjectModel *m_model = nullptr;
};

#endif // FRAMELISTWIDGET_H
