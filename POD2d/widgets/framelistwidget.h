#ifndef FRAMELISTWIDGET_H
#define FRAMELISTWIDGET_H

#include <QListWidget>
#include <QMenu>
#include <QResizeEvent>

namespace {
constexpr QSize kFrameThumb(128, 64);
constexpr QSize kFrameItem (140, 94);   // 3 + 64 + 2 + 16 + 3 = 88
}

class ProjectModel;

class FrameListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit FrameListWidget(QWidget *parent = nullptr);

    void setModel(ProjectModel *model);

public slots:
    void rebuildList();
    void reloadTheme();
    void setHorizontal(bool horizontal);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showContextMenu(const QPoint &pos);
    void onActiveFrameChanged(int index);
    void onRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void onCurrentRowChanged(int row);

private:
    ProjectModel *m_model = nullptr;

    void updateItemSizes();
    bool m_horizontal = true;
};

#endif // FRAMELISTWIDGET_H
