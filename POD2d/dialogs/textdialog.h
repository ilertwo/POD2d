#ifndef TEXTDIALOG_H
#define TEXTDIALOG_H

#include <QDialog>
#include <QFont>

namespace Ui {
class TextDialog;
}

class TextDialog : public QDialog {
    Q_OBJECT

public:
    explicit TextDialog(QWidget *parent = nullptr);
    ~TextDialog();

    QString getText() const;
    QFont getFont() const;

private slots:
    void updatePreview();

private:
    Ui::TextDialog *ui;
    QFont currentFont;
};

#endif // TEXTDIALOG_H
