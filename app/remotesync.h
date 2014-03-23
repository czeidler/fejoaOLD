#ifndef REMOTESYNC_H
#define REMOTESYNC_H

#include <QObject>

#include "databaseinterface.h"
#include "error_codes.h"
#include "profile.h"
#include "remoteconnectionmanager.h"

// preforms a push or pull
class RemoteSync : public RemoteConnectionJob    
{
Q_OBJECT
public:
    explicit RemoteSync(DatabaseInterface *database, RemoteDataStorage* remoteStorage, QObject *parent = 0);
    ~RemoteSync();

    virtual void run(RemoteConnectionJobQueue *jobQueue);
    virtual void abort();

    DatabaseInterface *getDatabase();

private slots:
    void syncConnected(WP::err code);
    void syncReply(WP::err code);
    void syncPushReply(WP::err code);

private:
    DatabaseInterface *database;
    RemoteDataStorage* remoteStorage;
    RemoteAuthenticationRef authentication;
    RemoteConnection *remoteConnection;
    RemoteConnectionReply *serverReply;

    QString syncUid;
};

typedef QSharedPointer<RemoteSync> RemoteSyncRef;

#endif // REMOTESYNC_H
