#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

namespace Ui {
class ExportDialog;
}

class ProjectModel;

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(ProjectModel *model, const QString &projName, QWidget *parent = nullptr);
    ~ExportDialog();

private slots:
    void generateCode();
    void copyToClipboard();
    void saveProjectFile();
    void exportToPng();
    void exportToGif();

private:
    Ui::ExportDialog *ui;
    ProjectModel *projectModel;
    QString projectName;
};

#endif // EXPORTDIALOG_H
