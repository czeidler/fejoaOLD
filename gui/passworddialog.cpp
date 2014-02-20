#include "passworddialog.h"
#include "ui_passworddialog.h"

#include <QPushButton>

PasswordDialog::PasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    connect(okButton, SIGNAL(clicked()), this, SLOT(onOkClicked()));
}

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

QString PasswordDialog::getPassword()
{
    return ui->passwordEdit->text();
}

void PasswordDialog::onOkClicked()
{
    done(QDialog::Accepted);
}
