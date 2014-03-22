#include "mainapplication.h"

#include "createprofiledialog.h"
#include "passworddialog.h"
#include "useridentity.h"


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

MainApplication::~MainApplication()
{
    delete mainWindow;
    delete profile;
    CryptoInterfaceSingleton::destroy();
}
