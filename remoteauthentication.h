#ifndef REMOTEAUTHENTICATION_H
#define REMOTEAUTHENTICATION_H

#include <QStringList>

#include "remoteconnection.h"

class Profile;

class RemoteAuthentication : public QObject
{
Q_OBJECT
public:
    RemoteAuthentication(RemoteConnection *connection);
    virtual ~RemoteAuthentication() {}

    RemoteConnection *getConnection();

    //! Establish a connection and login afterwards.
    WP::err login();
    void logout();
    bool isVerified();

protected slots:
    virtual void handleConnectionAttempt(WP::err code) = 0;

signals:
    void authenticationAttemptFinished(WP::err code);

protected:
    void setAuthenticationCanceled(WP::err code);
    void setAuthenticationSucceeded();

    virtual void getLogoutData(QByteArray &data) = 0;

    RemoteConnection *connection;
    RemoteConnectionReply *authenticationReply;

    bool authenticationInProgress;
    bool verified;
};


class SignatureAuthentication : public RemoteAuthentication {
Q_OBJECT
public:
    SignatureAuthentication(RemoteConnection *connection, Profile *profile,
                            const QString &userName, const QString &keyStoreId,
                            const QString &keyId, const QString &serverUser);

protected slots:
    void handleConnectionAttempt(WP::err code);
    void handleAuthenticationRequest(WP::err code);
    void handleAuthenticationAttempt(WP::err code);

protected:
    void getLoginRequestData(QByteArray &data);
    WP::err getLoginData(QByteArray &data, const QByteArray &serverRequest);
    WP::err wasLoginSuccessful(QByteArray &data);
    void getLogoutData(QByteArray &data);

private:
    Profile *profile;
    QString userName;
    QString serverUser;
    QString keyStoreId;
    QString keyId;

    QStringList roles;
};


#endif // REMOTEAUTHENTICATION_H
