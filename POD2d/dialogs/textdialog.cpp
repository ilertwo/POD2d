#include "textdialog.h"
#include "ui_textdialog.h"
#include <QPainter>
#include <QPixmap>

TextDialog::TextDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TextDialog)
{
    ui->setupUi(this);

    ui->weightCombo->setItemData(0, QFont::Light);
    ui->weightCombo->setItemData(1, QFont::Normal);
    ui->weightCombo->setItemData(2, QFont::Bold);
    ui->weightCombo->setItemData(3, QFont::Black);
    ui->weightCombo->setCurrentIndex(1);

    connect(ui->textEdit, &QLineEdit::textChanged, this, &TextDialog::updatePreview);
    connect(ui->sizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TextDialog::updatePreview);
    connect(ui->weightCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TextDialog::updatePreview);
    connect(ui->stretchSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &TextDialog::updatePreview);


    currentFont.setFamily("Arial");
    currentFont.setStyleStrategy(QFont::NoAntialias);
    updatePreview();
}

TextDialog::~TextDialog() {
    delete ui;
}

void TextDialog::updatePreview() {
    currentFont.setPixelSize(ui->sizeSpinBox->value());
    currentFont.setWeight(static_cast<QFont::Weight>(ui->weightCombo->currentData().toInt()));
    currentFont.setStretch(ui->stretchSpinBox->value());

    QString displayTxt = ui->textEdit->text().isEmpty() ? "Aa" : ui->textEdit->text();

    int w = qMax(1, ui->previewLabel->width());
    int h = qMax(1, ui->previewLabel->height());

    QPixmap pix(w, h);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::TextAntialiasing, false);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::white);
    p.setFont(currentFont);
    p.drawText(pix.rect(), Qt::AlignCenter, displayTxt);
    p.end();

    ui->previewLabel->setPixmap(pix);
}

QString TextDialog::getText() const { return ui->textEdit->text(); }
QFont TextDialog::getFont() const { return currentFont; }
