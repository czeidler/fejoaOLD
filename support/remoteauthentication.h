#ifndef REMOTEAUTHENTICATION_H
#define REMOTEAUTHENTICATION_H

#include <QStringList>

#include "remoteconnection.h"

class Profile;

class RemoteAuthenticationInfo {
public:
    enum RemoteAuthenticationType {
        kUnkown,
        kSignature
    };
    RemoteAuthenticationInfo();
    RemoteAuthenticationInfo(QString userName, QString serverUser, QString keyStoreId, QString keyId);

    friend bool operator== (const RemoteAuthenticationInfo &info1, const RemoteAuthenticationInfo &info2);
    friend bool operator!= (const RemoteAuthenticationInfo &info1, const RemoteAuthenticationInfo &info2);

    RemoteAuthenticationType getType() const;
    void setType(const RemoteAuthenticationType &value);

    QString getUserName() const;
    void setUserName(const QString &value);

    QString getServerUser() const;
    void setServerUser(const QString &value);

    QString getKeyStoreId() const;
    void setKeyStoreId(const QString &value);

    QString getKeyId() const;
    void setKeyId(const QString &value);

private:
    RemoteAuthenticationType type;
    QString userName;
    QString serverUser;
    QString keyStoreId;
    QString keyId;
};

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


class KeyStoreFinder;

class SignatureAuthentication : public RemoteAuthentication {
Q_OBJECT
public:
    SignatureAuthentication(RemoteConnection *connection, KeyStoreFinder *keyStoreFinder,
                            const RemoteAuthenticationInfo &info);

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
    KeyStoreFinder *keyStoreFinder;
    RemoteAuthenticationInfo authenticationInfo;

    QStringList roles;
};


#endif // REMOTEAUTHENTICATION_H
