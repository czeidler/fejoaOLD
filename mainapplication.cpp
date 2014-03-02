#include "mainapplication.h"

#include <QtNetwork/QNetworkCookieJar>

#include "createprofiledialog.h"
#include "passworddialog.h"
#include "useridentity.h"
#include "syncmanager.h"


#include <QFile>
QString readPassword() {
    QFile file("password");
    file.open(QIODevice::ReadOnly);
    char buffer[255];
    buffer[0] = '\0';
    file.readLine(buffer, 256);
    QString password(buffer);
    password.replace("\n", "");
    return password;
}

MainApplication::MainApplication(int &argc, char *argv[]) :
    QApplication(argc, argv)
{
    networkAccessManager = new QNetworkAccessManager(this);
    networkAccessManager->setCookieJar(new QNetworkCookieJar(this));

    profile = new Profile(".git", "profile");

    // convenient hack
    QString password = readPassword();

    WP::err error = WP::kBadKey;
    while (true) {
        error = profile->open(password.toLatin1());
        if (error != WP::kBadKey)
            break;

        PasswordDialog passwordDialog;
        int result = passwordDialog.exec();
        if (result != QDialog::Accepted) {
            exit(-1);
            return;
        }
        password = passwordDialog.getPassword();
    }

    if (error != WP::kOk) {
        CreateProfileDialog createDialog(profile);
        int result = createDialog.exec();
        if (result != QDialog::Accepted) {
            exit(-1);
            return;
        }
    }

    DatabaseBranch *branch = NULL;
    QList<DatabaseBranch*> &branches = profile->getBranches();
    syncManager = new SyncManager(branches.at(0)->getRemoteAt(0), this);
    foreach (branch, branches)
        syncManager->keepSynced(branch->getDatabase());
    syncManager->startWatching();
    connect(syncManager, SIGNAL(connectionError()), this, SLOT(onSyncError()));

    mainWindow = new MainWindow(profile);
    mainWindow->show();

    /*
    MessageReceiver receiver(&gitInterface);
    QDeclarativeView view;
    view.rootContext()->setContextProperty("messageReceiver", &receiver);
    view.setSource(QUrl::fromLocalFile("qml/woodpidgin/main.qml"));
    view.show();
*/

}

void MainApplication::onSyncError() {
    syncManager->startWatching();
}

MainApplication::~MainApplication()
{
    syncManager->stopWatching();

    delete mainWindow;
    delete profile;
    CryptoInterfaceSingleton::destroy();
}

QNetworkAccessManager *MainApplication::getNetworkAccessManager()
{
    return networkAccessManager;
}
