#ifndef REMOTECONNECTIONMANAGER_H
#define REMOTECONNECTIONMANAGER_H

#include <QSharedPointer>

#include "remoteauthentication.h"
#include "remoteconnection.h"

class RemoteConnectionJobQueue;

class RemoteConnectionJob : public QObject {
Q_OBJECT
public:
    RemoteConnectionJob(QObject *parent = NULL);
    virtual ~RemoteConnectionJob() {}

    virtual void run(RemoteConnectionJobQueue *jobQueue) = 0;
    virtual void abort() = 0;

signals:
    void jobDone(WP::err error);

};

typedef QSharedPointer<RemoteConnectionJob> RemoteConnectionJobRef;


class RemoteConnectionJobQueue : public QObject {
Q_OBJECT
public:
    RemoteConnectionJobQueue(RemoteConnection *connection = NULL);
    ~RemoteConnectionJobQueue();

    void start();

    void queue(RemoteConnectionJobRef job);
    void setIdleJob(RemoteConnectionJobRef job);

    RemoteConnection *getRemoteConnection() const;
    void setRemoteConnection(RemoteConnection *value);

    RemoteAuthentication *getRemoteAuthentication(const RemoteAuthenticationInfo &info, KeyStoreFinder *keyStoreFinder);

private slots:
    void onJobDone(WP::err error);

private:
    void startJob(RemoteConnectionJobRef job);
    void reschedule();

    RemoteConnection *remoteConnection;

    RemoteConnectionJobRef idleJob;

    RemoteConnectionJobRef runningJob;
    QList<RemoteConnectionJobRef> jobQueue;

    class AuthenticationEntry {
    public:
        AuthenticationEntry(const RemoteAuthenticationInfo &info, KeyStoreFinder *keyStoreFinder,
                            RemoteConnection *remoteConnection);

        RemoteAuthenticationInfo authenticationInfo;
        RemoteAuthentication *remoteAuthentication;
    };

    QList<AuthenticationEntry*> authenticationList;
};

class ConnectionManager {
public:
    ~ConnectionManager();

    RemoteConnectionJobQueue *getConnectionJobQueue(const RemoteConnectionInfo &info);

    static RemoteConnectionInfo getDefaultConnectionFor(const QUrl &url);
    static RemoteConnectionInfo getHTTPConnectionFor(const QUrl &url);
    static RemoteConnectionInfo getEncryptedPHPConnectionFor(const QUrl &url);

    static ConnectionManager *get();
private:
    ConnectionManager();

    class ConnectionEntry {
    public:
        ConnectionEntry(const RemoteConnectionInfo& info);

        RemoteConnectionInfo info;
        RemoteConnectionJobQueue jobQueue;

    private:
        RemoteConnection *createRemoteConnection(const RemoteConnectionInfo& info);
    };

    QList<ConnectionEntry*> connectionList;
};

#endif // REMOTECONNECTIONMANAGER_H
