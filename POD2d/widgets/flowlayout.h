#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QList>
#include <QRect>
#include <QSet>
#include <QSize>

class QLayoutItem;
class QWidget;

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = 0, int hSpacing = 0, int vSpacing = 0);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    void addBreak();

    void setGroupGap(int px);
    void setPreferredWidth(int px);

    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;

    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;

private:
    int doLayout(const QRect &rect, bool testOnly) const;

    QList<QLayoutItem *> m_items;
    QSet<QLayoutItem *>  m_breaks;
    int m_hSpace = 0;
    int m_vSpace = 0;
    int m_groupGap = 10;
    int m_preferredWidth = 0;
};

#endif // FLOWLAYOUT_H
