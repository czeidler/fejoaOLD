#include "createprofiledialog.h"
#include "ui_createprofiledialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include "profile.h"
#include "useridentity.h"


CreateProfileDialog::CreateProfileDialog(Profile *profile, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateProfileDialog),
    fProfile(profile)
{
    ui->setupUi(this);

    fApplyButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    fApplyButton->setEnabled(false);

    fServerUserEdit = ui->serverUserEdit;
    fServerEdit = ui->serverEdit;
    fPasswordEdit1 = ui->passwordEdit1;
    fPasswordEdit2 = ui->passwordEdit2;
    fPasswordLabel2 = ui->passwordLabel2;

    connect(fServerUserEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(fServerEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(fPasswordEdit1, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(fPasswordEdit2, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));

    connect(fApplyButton, SIGNAL(clicked()), this, SLOT(createProfile()));

    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::red);
    fPasswordLabel2->setPalette(palette);
}

CreateProfileDialog::~CreateProfileDialog()
{
    delete ui;
}

void CreateProfileDialog::onTextChanged(QString)
{
    fApplyButton->setEnabled(false);
    if (fPasswordEdit1->text() != "") {
        if (fPasswordEdit1->text() != fPasswordEdit2->text()) {
            QPalette palette;
            palette.setColor(QPalette::WindowText, Qt::red);
            fPasswordLabel2->setPalette(palette);
            return;
        } else {
            QPalette palette;
            palette.setColor(QPalette::WindowText, Qt::green);
            fPasswordLabel2->setPalette(palette);
        }
    } else
        return;

    if (fServerEdit->text() == "" || fServerUserEdit->text() == "")
        return;

    fApplyButton->setEnabled(true);
}

void CreateProfileDialog::createProfile()
{
    SecureArray password = fPasswordEdit1->text().toLatin1();
    QString serverUser = fServerUserEdit->text();
    QString server = fServerEdit->text();

    QString remoteUrl = "http://" + server + "/php_server/portal.php";

    WP::err error = fProfile->createNewProfile(password);
    if (error != WP::kOk) {
        QMessageBox::information(NULL, "Error", "Unable to create or load a profile!");
        exit(-1);
        return;
    }
    RemoteDataStorage *remote = fProfile->addHTTPRemote(remoteUrl);
    //RemoteDataStorage *remote = fProfile->addPHPRemote(remoteUrl);
    UserIdentity *mainIdentity = fProfile->getIdentityList()->identityAt(0);
    Contact *myself = mainIdentity->getMyself();
    myself->setServer(server);
    myself->setServerUser(serverUser);
    myself->writeConfig();
    fProfile->setSignatureAuth(remote, myself->getUid(),
                               mainIdentity->getKeyStore()->getUid(),
                               myself->getKeys()->getMainKeyId(), myself->getServerUser());

    fProfile->connectFreeBranches(remote);
    fProfile->commit();

    done(QDialog::Accepted);
}
