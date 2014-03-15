#ifndef REMOTECONNECTIONMANAGER_H
#define REMOTECONNECTIONMANAGER_H

#include "remoteauthentication.h"
#include "remoteconnection.h"


class RemoteConnectionJob : public QObject {
Q_OBJECT
public:
    RemoteConnectionJob(const RemoteConnectionInfo &info);
    virtual ~RemoteConnectionJob() {}

    virtual void run(RemoteConnection *connection) = 0;
    virtual void abort();

    RemoteConnectionInfo &getRemoteConnectionInfo();

signals:
    void jobDone(WP::err error);

private:
    RemoteConnectionInfo remoteConnectionInfo;
};


class RemoteConnectionJobQueue {
public:
    RemoteConnectionJobQueue(RemoteConnection *connection = NULL);
    ~RemoteConnectionJobQueue();

    void start();

    WP::err queue(RemoteConnectionJob *job);
    void setIdleJob(RemoteConnectionJob *job);

    RemoteConnection *getRemoteConnection() const;
    void setRemoteConnection(RemoteConnection *value);

private slots:
    void onJobDone(WP::err error);

private:
    void startJob(RemoteConnectionJob *job);
    void reschedule();

    RemoteConnection *remoteConnection;

    RemoteConnectionJob *idleJob;

    RemoteConnectionJob *runningJob;
    QList<RemoteConnectionJob*> jobQueue;
};

class ConnectionManager {
public:
    ~ConnectionManager();

    RemoteConnectionJobQueue *getConnectionJobQueue(const RemoteConnectionInfo &info);

    static RemoteConnectionInfo getDefaultConnectionFor(const QUrl &url);
    static RemoteConnectionInfo getHTTPConnectionFor(const QUrl &url);
    static RemoteConnectionInfo getEncryptedPHPConnectionFor(const QUrl &url);

    RemoteAuthentication *getRemoteAuthentication(const RemoteConnectionInfo &connection,
                                                  const RemoteAuthenticationInfo &authentication);

private:
    class AuthenticationEntry {
    public:
        AuthenticationEntry(const RemoteAuthenticationInfo &info);

        RemoteAuthenticationInfo info;
        RemoteAuthentication *remoteAuthentication;
    };

    class ConnectionEntry {
    public:
        ConnectionEntry(const RemoteConnectionInfo& info);

        RemoteConnectionInfo info;
        RemoteConnectionJobQueue jobQueue;

        AuthenticationEntry &getAuthenticationEntry(const RemoteAuthenticationInfo &info);
    private:
        RemoteConnection *createRemoteConnection(const RemoteConnectionInfo& info);

        QList<AuthenticationEntry> authenticationList;
    };

    QList<ConnectionEntry> connectionList;
};

#endif // REMOTECONNECTIONMANAGER_H
