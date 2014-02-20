#ifndef REMOTESYNC_H
#define REMOTESYNC_H

#include <QObject>

#include "databaseinterface.h"
#include "error_codes.h"
#include "profile.h"
#include "remoteconnection.h"


class RemoteSync : public QObject
{
    Q_OBJECT
public:
    explicit RemoteSync(DatabaseInterface *database, RemoteDataStorage* remoteStorage, QObject *parent = 0);
    ~RemoteSync();

    WP::err sync();

signals:
    void syncFinished(WP::err status);

private slots:
    void syncConnected(WP::err code);
    void syncReply(WP::err code);
    void syncPushReply(WP::err code);

private:
    DatabaseInterface *fDatabase;
    RemoteDataStorage* fRemoteStorage;
    RemoteAuthentication *fAuthentication;
    RemoteConnection *fRemoteConnection;
    RemoteConnectionReply *fServerReply;

    QString fSyncUid;
};

#endif // REMOTESYNC_H
