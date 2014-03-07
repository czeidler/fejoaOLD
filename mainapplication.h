#ifndef MAINAPPLICATION_H
#define MAINAPPLICATION_H

#include <QApplication>
#include <QtNetwork/QNetworkAccessManager>

#include <cryptointerface.h>
#include <mainwindow.h>
#include <profile.h>


class DatabaseInterface;

#include <remoteconnection.h>

class PingRCCommand : public QObject
{
Q_OBJECT
public:
    PingRCCommand(RemoteConnection *connection, QObject *parent = NULL) :
        QObject(parent),
        networkConnection(connection)
    {
    }

public slots:
    void connectionAttemptFinished(WP::err code)
    {
        if (code != WP::kOk)
            return;

        QByteArray data("ping");
        reply = networkConnection->send(data);
        connect(reply, SIGNAL(finished()), this, SLOT(received()));
    }

    virtual void received()
    {
        QByteArray data = reply->readAll();
    }

private:
    RemoteConnection* networkConnection;
    RemoteConnectionReply *reply;
};


class MainApplication : public QApplication
{
Q_OBJECT
public:
    MainApplication(int &argc, char *argv[]);
    ~MainApplication();

    QNetworkAccessManager* getNetworkAccessManager();

private:
    WP::err createNewProfile();

    MainWindow *mainWindow;

    Profile* profile;
    QNetworkAccessManager *networkAccessManager;
};

#endif // MAINAPPLICATION_H
