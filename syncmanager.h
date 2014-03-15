#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>

#include "remotestorage.h"
#include "remotesync.h"


class SyncManager;

class SyncEntry : public QObject {
    Q_OBJECT
public:
    SyncEntry(SyncManager* syncManager, DatabaseInterface *database,
              QObject *parent = NULL);

    WP::err sync();
    bool isSyncing();

    const DatabaseInterface *getDatabase() const;

public slots:
    void onSyncFinished(WP::err error);

private:
    SyncManager* syncManager;
    DatabaseInterface *database;
    RemoteSync *remoteSync;
    QString oldTip;
};


class SyncManager : public QObject
{
    Q_OBJECT
public:
    explicit SyncManager(RemoteDataStorage* remoteDataStorage, QObject *parent = 0);
    ~SyncManager();

    /*! Start monitoring a remote branch. If changes are detected the branch is synced.*/
    WP::err keepSynced(DatabaseInterface *branchDatabase);

    void startWatching();
    void stopWatching();

private:
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
    QList<SyncEntry*> syncEntries;

    RemoteAuthentication *authentication;
    RemoteConnection *remoteConnection;
    RemoteConnectionReply *serverReply;

    bool watching;
};

#endif // SYNCMANAGER_H
