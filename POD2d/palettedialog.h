#ifndef PALETTEDIALOG_H
#define PALETTEDIALOG_H

#include <QDialog>
#include <QList>
#include <QColor>

namespace Ui {
class PaletteDialog;
}

class PaletteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PaletteDialog(QWidget *parent = nullptr);
    ~PaletteDialog();

    // Methods for transferring data back and forth
    void setPaletteData(const QList<QColor> &palette, int activeIndex);
    QList<QColor> getPalette() const;

    void syncColorToUI(bool updateSpins = true);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::PaletteDialog *ui;

    // Internal variables
    QList<QColor> m_palette;
    int m_activeIndex;
    QColor m_currentColor;

    // Functions for updating the interface
    void rebuildGrid();
    void updateSpectrums();
    void updateHex();
};

#endif // PALETTEDIALOG_H
