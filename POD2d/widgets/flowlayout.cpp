#include "flowlayout.h"

#include <QSpacerItem>
#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    qDeleteAll(m_items);
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}

void FlowLayout::addBreak()
{
    auto *br = new QSpacerItem(0, 0);
    m_breaks.insert(br);
    m_items.append(br);
}

void FlowLayout::setGroupGap(int px)
{
    m_groupGap = px;
    invalidate();
}

void FlowLayout::setPreferredWidth(int px)
{
    m_preferredWidth = px;
    invalidate();
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return m_items.value(index, nullptr);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size())
        return nullptr;
    QLayoutItem *it = m_items.takeAt(index);
    m_breaks.remove(it);
    return it;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    const int w = qMax(m_preferredWidth, minimumSize().width());
    return QSize(w, heightForWidth(w));
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (QLayoutItem *item : m_items) {
        if (m_breaks.contains(item) || item->isEmpty())
            continue;
        size = size.expandedTo(item->minimumSize());
    }
    const QMargins m = contentsMargins();
    return size + QSize(m.left() + m.right(), m.top() + m.bottom());
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    const QMargins m = contentsMargins();
    const QRect eff = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());

    int x = eff.x(), y = eff.y(), lineHeight = 0;
    bool lineStarted = false, pendingBreak = false;

    for (QLayoutItem *item : m_items) {
        if (m_breaks.contains(item)) {
            if (lineStarted) pendingBreak = true;
            continue;
        }
        if (item->isEmpty())
            continue;

        const QSize sz = item->sizeHint();

        if (pendingBreak) {
            x = eff.x();
            y += lineHeight + m_groupGap;
            lineHeight = 0;
            pendingBreak = false;
            lineStarted = false;
        } else if (lineStarted && x + sz.width() > eff.right() + 1) {
            x = eff.x();
            y += lineHeight + m_vSpace;
            lineHeight = 0;
            lineStarted = false;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), sz));

        x += sz.width() + m_hSpace;
        lineHeight = qMax(lineHeight, sz.height());
        lineStarted = true;
    }
    return y + lineHeight + m.bottom() - rect.y();
}
