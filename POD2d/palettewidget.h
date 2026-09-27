#ifndef PALETTEWIDGET_H
#define PALETTEWIDGET_H

#include <QWidget>
#include <QColor>
#include <QList>

class QGridLayout;

class PaletteWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaletteWidget(QWidget *parent = nullptr);

    void setPalette(const QList<QColor> &palette);
    QList<QColor> getPalette() const;
    void reloadTheme();

signals:
    void primaryColorSelected(const QColor &color);
    void secondaryColorSelected(const QColor &color);
    void requestEditColor(int index);
    void paletteChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onAddNewColorClicked();

private:
    void rebuildGrid();
    void savePaletteToSettings();
    void loadPaletteFromSettings();

    QList<QColor> m_palette;
    QGridLayout *m_gridLayout;
    int m_mainSwapIndex = -1;
};

#endif // PALETTEWIDGET_H
