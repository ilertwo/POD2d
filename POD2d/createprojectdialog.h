#ifndef CREATEPROJECTDIALOG_H
#define CREATEPROJECTDIALOG_H

#include <QDialog>

namespace Ui {
class CreateProjectDialog;
}

class CreateProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateProjectDialog(QWidget *parent = nullptr);
    ~CreateProjectDialog();

    int getWidth() const;
    int getHeight() const;
    QString getProjectName() const;
    QString getFullFilePath() const;
    void on_btn_Browse_clicked();

    bool isRGBMode() const;

private:
    Ui::CreateProjectDialog *ui;
};

#endif
