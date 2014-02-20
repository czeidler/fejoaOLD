#ifndef REMOTESTORAGE_H
#define REMOTESTORAGE_H

#include "databaseutil.h"
#include "remoteconnection.h"


class Profile;
class RemoteAuthentication;

class RemoteDataStorage {
public:
    RemoteDataStorage(Profile *profile);
    virtual ~RemoteDataStorage();

    WP::err write(StorageDirectory &dir);
    WP::err load(StorageDirectory &dir);

    QString getUid();

    const QString &getConnectionType();
    const QString &getUrl();

    const QString &getAuthType();
    const QString &getAuthUserName();
    const QString &getAuthKeyStoreId();
    const QString &getAuthKeyId();
    const QString &getServerUser();

    RemoteConnection *getRemoteConnection();
    RemoteAuthentication *getRemoteAuthentication();

private:
    friend class Profile;

    void setPHPEncryptedRemoteConnection(const QString &url);
    void setHTTPRemoteConnection(const QString &url);

    void setSignatureAuth(const QString &userName, const QString &keyStoreId, const QString &keyId, const QString &serverUser);

    QString hash();

    Profile *fProfile;

    RemoteConnection *fConnection;
    RemoteAuthentication *fAuthentication;

    QString fConnectionType;
    QString fUrl;
    QString fUid;

    QString fServerUser;

    QString fAuthType;
    QString fAuthUserName;
    QString fAuthKeyStoreId;
    QString fAuthKeyId;
};

#endif // REMOTESTORAGE_H
