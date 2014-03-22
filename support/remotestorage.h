#ifndef REMOTESTORAGE_H
#define REMOTESTORAGE_H

#include "databaseutil.h"
#include "remoteauthentication.h"
#include "remoteconnection.h"


class Profile;
class RemoteAuthentication;

/*!
 * \brief The RemoteDataStorage class hold all the informations for a remote storage branch.
 * For example, where the storage is located and what authentication is to be used.
 */
class RemoteDataStorage {
public:
    RemoteDataStorage(KeyStoreFinder *keyStoreFinder);

    WP::err write(StorageDirectory &dir);
    WP::err load(StorageDirectory &dir);

    QString getUid();

    const QString &getConnectionType();
    const QString &getAuthType();

    KeyStoreFinder *getKeyStoreFinder();

    RemoteConnectionInfo getRemoteConnectionInfo();
    RemoteAuthenticationInfo getRemoteAuthenticationInfo();

    void setPHPEncryptedRemoteConnection(const QString &url);
    void setHTTPRemoteConnection(const QString &url);

    void setSignatureAuth(const QString &userName, const QString &keyStoreId, const QString &keyId, const QString &serverUser);

private:
    QString hash();

    RemoteConnectionInfo connectionInfo;
    RemoteAuthenticationInfo authenticationInfo;

    QString connectionType;
    QString uid;

    QString authType;
    KeyStoreFinder *keyStoreFinder;
};

#endif // REMOTESTORAGE_H
