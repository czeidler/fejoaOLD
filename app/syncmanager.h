#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>

#include "remotestorage.h"
#include "remotesync.h"


class SyncManager : public RemoteConnectionJob
{
Q_OBJECT
public:
    explicit SyncManager(RemoteDataStorage* remoteDataStorage, QObject *parent = 0);
    ~SyncManager();

    /*! Start monitoring a remote branch. If changes are detected the branch is synced.*/
    WP::err keepSynced(DatabaseInterface *branchDatabase);

    virtual void run(RemoteConnectionJobQueue *jobQueue);
    virtual void abort();

private:
    void startWatching();
    void restartWatching();

    void syncBranches(const QStringList &branches);
    void handleConnectionError(WP::err error);

signals:
    void syncStarted();
    void syncStopped();
    void connectionError();

private slots:
    void remoteAuthenticated(WP::err error);
    void watchReply(WP::err error);

private:
    friend class SyncEntry;
    friend class PauseLock;

    RemoteDataStorage* remoteDataStorage;
    QList<RemoteSyncRef> syncEntries;

    RemoteAuthenticationRef authentication;
    RemoteConnection *remoteConnection;
    RemoteConnectionReply *serverReply;

    RemoteConnectionJobQueue *jobQueue;

    bool watching;
};

typedef QSharedPointer<SyncManager> SyncManagerRef;

#endif // SYNCMANAGER_H
