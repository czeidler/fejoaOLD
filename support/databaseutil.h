#ifndef DATABASEUTIL_H
#define DATABASEUTIL_H

#include <qobject.h>

#include "cryptointerface.h"
#include "databaseinterface.h"
#include "diffmonitor.h"
#include "error_codes.h"


class EncryptedUserData;

class StorageDirectory {
public:
    StorageDirectory(EncryptedUserData *database, const QString &directory);

    void setTo(EncryptedUserData *database, const QString &directory);
    void setTo(StorageDirectory *storageDir);

    WP::err write(const QString& path, const QByteArray& data);
    WP::err write(const QString& path, const QString& data);
    WP::err read(const QString& path, QByteArray& data) const;
    WP::err read(const QString& path, QString& data) const;
    WP::err writeSafe(const QString& path, const QString& data);
    WP::err writeSafe(const QString& path, const QByteArray& data);
    WP::err writeSafe(const QString& path, const QByteArray& data, const QString &keyId);
    WP::err readSafe(const QString& path, QString& data) const;
    WP::err readSafe(const QString& path, QByteArray& data) const;
    WP::err readSafe(const QString& path, QByteArray& data, const QString &keyId) const;

    WP::err remove(const QString& path);

    const QString& getDirectory();
    void setDirectory(const QString &directory);

    QStringList listDirectories(const QString &path) const;
protected:
    EncryptedUserData *database;
    QString directory;
};


class RemoteDataStorage;
class RemoteConnection;
class RemoteAuthentication;

class DatabaseBranch {
public:
    DatabaseBranch(const QString &databasePath, const QString &getBranch);
    ~DatabaseBranch();

    void setTo(const QString &databasePath, const QString &getBranch);
    const QString &getDatabasePath() const;
    const QString &getBranch() const;
    QString databaseHash() const;
    DatabaseInterface *getDatabase() const;

    WP::err commit();
    int countRemotes() const;
    RemoteDataStorage *getRemoteAt(int i) const;
    RemoteConnection *getRemoteConnectionAt(int i) const;
    RemoteAuthentication *getRemoteAuthAt(int i) const;
    WP::err addRemote(RemoteDataStorage* data);
private:
    QString databasePath;
    QString branch;
    QList<RemoteDataStorage*> remotes;
    DatabaseInterface *database;
};


class UserData : public QObject {
Q_OBJECT
public:
    UserData();
    virtual ~UserData();

    virtual WP::err writeConfig();

    QString getUid();

    // convenient functions to DatabaseInterface
    WP::err write(const QString& path, const QByteArray& data);
    WP::err write(const QString& path, const QString& data);
    WP::err read(const QString& path, QByteArray& data) const;
    WP::err read(const QString& path, QString& data) const;
    WP::err remove(const QString& path);

    QStringList listDirectories(const QString &path) const;
    QStringList listFiles(const QString &path) const;

    DatabaseBranch *getDatabaseBranch() const;
    QString getDatabaseBaseDir() const;
    DatabaseInterface *getDatabase() const;
    void setBaseDir(const QString &baseDir);

    DiffMonitor* getDiffMonitor();

protected:
    void setUid(const QString &uid);
    WP::err setToDatabase(DatabaseBranch *branch, const QString &baseDir = "");

    QString prependBaseDir(const QString &path) const;

protected:
    DatabaseBranch *databaseBranch;
    QString databaseBaseDir;

    CryptoInterface *crypto;
    DatabaseInterface *database;

private:
    QString uid;
    DiffMonitor diffMonitor;
};

class KeyStore : public UserData {
public:
    KeyStore(DatabaseBranch *branch, const QString &baseDir = "");

    WP::err open(const SecureArray &password);
    WP::err create(const SecureArray &password, bool addUidToBaseDir = true);

    WP::err writeSymmetricKey(const SecureArray &key, const QByteArray &iv, QString &keyId);
    WP::err readSymmetricKey(const QString &keyId, SecureArray &key, QByteArray &iv);

    WP::err writeAsymmetricKey(const QString &certificate, const QString &publicKey,
                         const QString &privateKey, QString &keyId);
    WP::err readAsymmetricKey(const QString &keyId, QString &certificate, QString &publicKey,
                         QString &privateKey);

    WP::err removeKey(const QString &id);

    CryptoInterface* getCryptoInterface();
    DatabaseInterface* getDatabaseInterface();

protected:
    SecureArray masterKey;
    QByteArray masterKeyIV;
};

class KeyStoreFinder {
public:
    virtual ~KeyStoreFinder() {}
    virtual KeyStore *find(const QString &keyStoreId) = 0;
};

class EncryptedUserData : public UserData {
public:
    EncryptedUserData(const EncryptedUserData& data);
    EncryptedUserData();
    virtual ~EncryptedUserData();

    virtual WP::err writeConfig();
    virtual WP::err open(KeyStoreFinder *keyStoreFinder);

    KeyStore *getKeyStore() const;
    void setKeyStore(KeyStore *keyStore);

    QString getDefaultKeyId() const;
    void setDefaultKeyId(const QString &keyId);

    // add and get encrypted data using the default key
    WP::err writeSafe(const QString& path, const QByteArray& data);
    WP::err readSafe(const QString& path, QByteArray& data) const;
    WP::err writeSafe(const QString& path, const QString& data);
    WP::err readSafe(const QString& path, QString& data) const;
    // add and get encrypted data
    WP::err writeSafe(const QString& path, const QString& data, const QString &keyId);
    WP::err writeSafe(const QString& path, const QByteArray& data, const QString &keyId);
    WP::err readSafe(const QString& path, QString& data, const QString &keyId) const;
    WP::err readSafe(const QString& path, QByteArray& data, const QString &keyId) const;

protected:
    virtual WP::err create(const QString &uid, KeyStore *keyStore, const QString defaultKeyId,
                           bool addUidToBaseDir);

    KeyStore *keyStore;
    QString defaultKeyId;
};

#endif // DATABASEUTIL_H
