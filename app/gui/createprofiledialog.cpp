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
    profile(profile)
{
    ui->setupUi(this);

    applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    applyButton->setEnabled(false);

    serverUserEdit = ui->serverUserEdit;
    serverEdit = ui->serverEdit;
    passwordEdit1 = ui->passwordEdit1;
    passwordEdit2 = ui->passwordEdit2;
    passwordLabel2 = ui->passwordLabel2;

    connect(serverUserEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(serverEdit, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(passwordEdit1, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));
    connect(passwordEdit2, SIGNAL(textChanged(QString)), this, SLOT(onTextChanged(QString)));

    connect(applyButton, SIGNAL(clicked()), this, SLOT(createProfile()));

    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::red);
    passwordLabel2->setPalette(palette);
}

CreateProfileDialog::~CreateProfileDialog()
{
    delete ui;
}

void CreateProfileDialog::onTextChanged(QString)
{
    applyButton->setEnabled(false);
    if (passwordEdit1->text() != "") {
        if (passwordEdit1->text() != passwordEdit2->text()) {
            QPalette palette;
            palette.setColor(QPalette::WindowText, Qt::red);
            passwordLabel2->setPalette(palette);
            return;
        } else {
            QPalette palette;
            palette.setColor(QPalette::WindowText, Qt::green);
            passwordLabel2->setPalette(palette);
        }
    } else
        return;

    if (serverEdit->text() == "" || serverUserEdit->text() == "")
        return;

    applyButton->setEnabled(true);
}

void CreateProfileDialog::createProfile()
{
    SecureArray password = passwordEdit1->text().toLatin1();
    QString serverUser = serverUserEdit->text();
    QString server = serverEdit->text();

    QString remoteUrl = "http://" + server + "/php_server/portal.php";

    WP::err error = profile->createNewProfile(password);
    if (error != WP::kOk) {
        QMessageBox::information(NULL, "Error", "Unable to create or load a profile!");
        exit(-1);
        return;
    }
    RemoteDataStorage *remote = profile->addHTTPRemote(remoteUrl);
    //RemoteDataStorage *remote = fProfile->addPHPRemote(remoteUrl);
    UserIdentity *mainIdentity = profile->getIdentityList()->identityAt(0);
    Contact *myself = mainIdentity->getMyself();
    myself->setServer(server);
    myself->setServerUser(serverUser);
    myself->writeConfig();
    profile->setSignatureAuth(remote, myself->getUid(),  myself->getServerUser(),
                               mainIdentity->getKeyStore()->getUid(),
                               myself->getKeys()->getMainKeyId());

    profile->connectFreeBranches(remote);
    profile->commit();

    done(QDialog::Accepted);
}
