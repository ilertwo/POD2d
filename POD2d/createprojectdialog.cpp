#include "createprojectdialog.h"
#include "ui_createprojectdialog.h"

CreateProjectDialog::CreateProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateProjectDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Create project");

    connect(ui->btn_ConfirmCreate, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btn_Cancel, &QPushButton::clicked, this, &QDialog::reject);
}

CreateProjectDialog::~CreateProjectDialog()
{
    delete ui;
}

QString CreateProjectDialog::getProjectName() const {
    return ui->input_ProjectName->toPlainText().trimmed();
}
