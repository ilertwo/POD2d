#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

signals:
    void settingsApplied();

private slots:
    void updateScaleLabel(int value);
    void chooseGridColor();

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void setActiveTab(int index);

    void loadSettings();
    void saveSettings();
    void setButtonColor(const QString &hexColor);

private:
    Ui::SettingsDialog *ui;

    QString currentGridColor;
};

#endif // SETTINGSDIALOG_H
