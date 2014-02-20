#include "databaseutil.h"

#include <QStringList>
#include <QTextStream>
#include <QUuid>

#include <remotestorage.h>


const char* kPathMasterKey = "master_key";
const char* kPathMasterKeyIV = "master_key_iv";
const char* kPathMasterPasswordKDF = "master_password_kdf";
const char* kPathMasterPasswordAlgo = "master_password_algo";
const char* kPathMasterPasswordSalt = "master_password_salt";
const char* kPathMasterPasswordSize = "master_password_size";
const char* kPathMasterPasswordIterations = "master_password_iterations";

const int kMasterPasswordIterations = 20000;
const int kMasterPasswordLength = 256;

const char *kPathSymmetricKey = "symmetric_key";
const char *kPathSymmetricIV = "symmetric_iv";
const char *kPathPrivateKey = "private_key";
const char *kPathPublicKey = "public_key";
const char *kPathCertificate = "certificate";

const char *kPathUniqueId = "uid";

UserData::UserData() :
    fDatabase(NULL)
{
    fCrypto = CryptoInterfaceSingleton::getCryptoInterface();
}

UserData::~UserData()
{
}

WP::err UserData::writeConfig()
{
    if (fDatabase == NULL)
        return WP::kNotInit;
    WP::err status = write(kPathUniqueId, fUid);
    if (status != WP::kOk)
        return status;
    return WP::kOk;
}

QString UserData::getUid()
{
    if (fUid == "")
        read(kPathUniqueId, fUid);

    return fUid;
}

WP::err UserData::write(const QString &path, const QByteArray &data)
{
    return fDatabase->write(prependBaseDir(path), data);
}

WP::err UserData::write(const QString &path, const QString &data)
{
    return fDatabase->write(prependBaseDir(path), data);
}

WP::err UserData::read(const QString &path, QByteArray &data) const
{
    return fDatabase->read(prependBaseDir(path), data);
}

WP::err UserData::read(const QString &path, QString &data) const
{
    return fDatabase->read(prependBaseDir(path), data);
}

WP::err UserData::remove(const QString &path)
{
    return fDatabase->remove(prependBaseDir(path));
}

QStringList UserData::listDirectories(const QString &path) const
{
    return fDatabase->listDirectories(prependBaseDir(path));
}

QStringList UserData::listFiles(const QString &path) const
{
    return fDatabase->listFiles(prependBaseDir(path));
}

DatabaseBranch *UserData::getDatabaseBranch() const
{
    return fDatabaseBranch;
}

QString UserData::getDatabaseBaseDir() const
{
    return fDatabaseBaseDir;
}

DatabaseInterface *UserData::getDatabase() const
{
    return fDatabase;
}

void UserData::setUid(const QString &uid)
{
    fUid = uid;
}

WP::err UserData::setToDatabase(DatabaseBranch *branch, const QString &baseDir)
{
    fDatabaseBranch = branch;
    fDatabaseBaseDir = baseDir;
    fDatabase = branch->getDatabase();
    if (fDatabase == NULL)
        return WP::kError;
    connect(fDatabase, SIGNAL(newCommits(QString,QString)), this, SLOT(onNewCommits(QString,QString)));
    return WP::kOk;
}

void UserData::setBaseDir(const QString &baseDir)
{
    fDatabaseBaseDir = baseDir;
}

void UserData::onNewCommits(const QString &startCommit, const QString &endCommit)
{

}

QString UserData::prependBaseDir(const QString &path) const
{
    if (fDatabaseBaseDir == "")
        return path;
    QString completePath = path;
    completePath.prepend(fDatabaseBaseDir + "/");
    return completePath;
}


KeyStore::KeyStore(DatabaseBranch *branch, const QString &baseDir)
{
    setToDatabase(branch, baseDir);
}

WP::err KeyStore::open(const SecureArray &password)
{
    // write master password (master password is encrypted
    QByteArray encryptedMasterKey;
    WP::err error = read(kPathMasterKey, encryptedMasterKey);
    if (error != WP::kOk)
        return error;
    error = read(kPathMasterKeyIV, fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    QByteArray kdfName;
    error = read(kPathMasterPasswordKDF, kdfName);
    if (error != WP::kOk)
        return error;
    QByteArray algoName;
    error = read(kPathMasterPasswordAlgo, algoName);
    if (error != WP::kOk)
        return error;
    QByteArray salt;
    error = read(kPathMasterPasswordSalt, salt);
    if (error != WP::kOk)
        return error;
    QByteArray masterPasswordSize;
    error = read(kPathMasterPasswordSize, masterPasswordSize);
    if (error != WP::kOk)
        return error;
    QByteArray masterPasswordIterations;
    error = read(kPathMasterPasswordIterations, masterPasswordIterations);
    if (error != WP::kOk)
        return error;

    QTextStream sizeStream(masterPasswordSize);
    unsigned int keyLength;
    sizeStream >> keyLength;
    QTextStream iterationsStream(masterPasswordIterations);
    unsigned int iterations;
    iterationsStream >> iterations;
    // key to encrypte the master key
    SecureArray passwordKey = fCrypto->deriveKey(password, kdfName, algoName, salt, keyLength, iterations);
    // key to encrypte all other data

    return fCrypto->decryptSymmetric(encryptedMasterKey, fMasterKey, passwordKey, fMasterKeyIV);
}

WP::err KeyStore::create(const SecureArray &password, bool addUidToBaseDir)
{
    QByteArray salt = fCrypto->generateSalt(QUuid::createUuid().toString());
    const QString kdfName = "pbkdf2";
    const QString algoName = "sha1";

    SecureArray passwordKey = fCrypto->deriveKey(password, kdfName, algoName, salt,
                                               kMasterPasswordLength, kMasterPasswordIterations);

    fMasterKeyIV = fCrypto->generateInitalizationVector(kMasterPasswordLength);
    fMasterKey = fCrypto->generateSymmetricKey(kMasterPasswordLength);

    QByteArray encryptedMasterKey;
    WP::err error = fCrypto->encryptSymmetric(fMasterKey, encryptedMasterKey, passwordKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;

    QString uid = fCrypto->toHex(fCrypto->sha1Hash(encryptedMasterKey));
    if (addUidToBaseDir) {
        QString newBaseDir;
        (fDatabaseBaseDir == "") ? newBaseDir = uid : newBaseDir = fDatabaseBaseDir + "/" + uid;
        setBaseDir(newBaseDir);
    }
    setUid(uid);

    error = UserData::writeConfig();
    if (error != WP::kOk)
        return error;

    // write master password (master password is encrypted
    write(kPathMasterKey, encryptedMasterKey);
    write(kPathMasterKeyIV, fMasterKeyIV);
    write(kPathMasterPasswordKDF, kdfName.toLatin1());
    write(kPathMasterPasswordAlgo, algoName.toLatin1());
    write(kPathMasterPasswordSalt, salt);
    QString keyLengthString;
    QTextStream(&keyLengthString) << kMasterPasswordLength;
    write(kPathMasterPasswordSize, keyLengthString.toLatin1());
    QString iterationsString;
    QTextStream(&iterationsString) << kMasterPasswordIterations;
    write(kPathMasterPasswordIterations, iterationsString.toLatin1());

    return WP::kOk;
}


WP::err KeyStore::writeSymmetricKey(const SecureArray &key, const QByteArray &iv, QString &keyId)
{
    QByteArray encryptedKey;
    WP::err error = fCrypto->encryptSymmetric(key, encryptedKey, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    keyId = fCrypto->toHex(fCrypto->sha1Hash(encryptedKey));

    QString path = keyId + "/" + kPathSymmetricKey;
    error = write(path, encryptedKey);
    if (error != WP::kOk)
        return error;
    path = keyId + "/" + kPathSymmetricIV;
    error = write(path, iv);
    if (error != WP::kOk) {
        remove(keyId);
        return error;
    }
    return WP::kOk;
}

WP::err KeyStore::readSymmetricKey(const QString &keyId, SecureArray &key, QByteArray &iv)
{
    QByteArray encryptedKey;
    QString path = keyId + "/" + kPathSymmetricKey;
    WP::err error = read(path, encryptedKey);
    if (error != WP::kOk)
        return error;
    path = keyId + "/" + kPathSymmetricIV;
    error = read(path, iv);
    if (error != WP::kOk)
        return error;
    return fCrypto->decryptSymmetric(encryptedKey, key, fMasterKey, fMasterKeyIV);
}

WP::err KeyStore::writeAsymmetricKey(const QString &certificate, const QString &publicKey,
                               const QString &privateKey, QString &keyId)
{
    QByteArray encryptedPrivate;
    WP::err error = fCrypto->encryptSymmetric(privateKey.toLatin1(), encryptedPrivate, fMasterKey,
                                          fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    /*QByteArray encryptedPublic;
    error = fCrypto->encryptSymmetric(publicKey.toLatin1(), encryptedPublic, fMasterKey,
                                      fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    QByteArray encryptedCertificate;
    error = fCrypto->encryptSymmetric(certificate.toLatin1(), encryptedCertificate, fMasterKey,
                                      fMasterKeyIV);
    if (error != WP::kOk)
        return error;*/

    keyId = fCrypto->toHex(fCrypto->sha1Hash(publicKey.toLatin1()));
    QString path = keyId + "/" + kPathPrivateKey;
    error = write(path, encryptedPrivate);
    if (error != WP::kOk)
        return error;
    path = keyId + "/" + kPathPublicKey;
    error = write(path, publicKey);
    if (error != WP::kOk) {
        remove(keyId);
        return error;
    }
    path = keyId + "/" + kPathCertificate;
    error = write(path, certificate);
    if (error != WP::kOk) {
        remove(keyId);
        return error;
    }
    return WP::kOk;

}

WP::err KeyStore::readAsymmetricKey(const QString &keyId, QString &certificate, QString &publicKey, QString &privateKey)
{
    QString path = keyId + "/" + kPathPrivateKey;
    QByteArray encryptedPrivate;
    WP::err error = read(path, encryptedPrivate);
    if (error != WP::kOk)
        return error;
    path = keyId + "/" + kPathPublicKey;
    //QByteArray encryptedPublic;
    error = read(path, publicKey);
    if (error != WP::kOk)
        return error;
    path = keyId + "/" + kPathCertificate;
    //QByteArray encryptedCertificate;
    error = read(path, certificate);
    if (error != WP::kOk)
        return error;

    SecureArray decryptedPrivate;
    //SecureArray decryptedPublic;
    //SecureArray decryptedCertificate;

    error = fCrypto->decryptSymmetric(encryptedPrivate, decryptedPrivate, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    /*error = fCrypto->decryptSymmetric(encryptedPublic, decryptedPublic, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;
    error = fCrypto->decryptSymmetric(encryptedCertificate, decryptedCertificate, fMasterKey, fMasterKeyIV);
    if (error != WP::kOk)
        return error;*/

    privateKey = decryptedPrivate;
    //publicKey = decryptedPublic;
    //certificate = decryptedCertificate;
    return WP::kOk;
}

CryptoInterface *KeyStore::getCryptoInterface()
{
    return fCrypto;
}

DatabaseInterface *KeyStore::getDatabaseInterface()
{
    return fDatabase;
}

const char *kPathKeyStoreId = "key_store_id";
const char *kPathDefaultKeyId = "default_key_id";

EncryptedUserData::EncryptedUserData(const EncryptedUserData &data) :
    fKeyStore(data.getKeyStore()),
    fDefaultKeyId(data.getDefaultKeyId())
{
    setToDatabase(data.getDatabaseBranch(), data.getDatabaseBaseDir());
}

EncryptedUserData::EncryptedUserData() :
    fKeyStore(NULL)
{
}

EncryptedUserData::~EncryptedUserData()
{
}

WP::err EncryptedUserData::writeConfig()
{
    if (fKeyStore == NULL)
        return WP::kNotInit;

    WP::err error = UserData::writeConfig();
    if (error != WP::kOk)
        return error;

    error = write(kPathKeyStoreId, fKeyStore->getUid());
    if (error != WP::kOk)
        return error;

    error = write(kPathDefaultKeyId, fDefaultKeyId);
    return error;
}

WP::err EncryptedUserData::open(KeyStoreFinder *keyStoreFinder)
{
    if (fDatabase == NULL)
        return WP::kNotInit;

    QString keyStoreId;
    WP::err error = read(kPathKeyStoreId, keyStoreId);
    if (error != WP::kOk)
        return error;
    fKeyStore = keyStoreFinder->find(keyStoreId);
    if (fKeyStore == NULL)
        return WP::kEntryNotFound;

    error = read(kPathDefaultKeyId, fDefaultKeyId);
    return error;
}

KeyStore *EncryptedUserData::getKeyStore() const
{
    return fKeyStore;
}

WP::err EncryptedUserData::writeSafe(const QString &path, const QByteArray &data)
{
    return writeSafe(path, data, fDefaultKeyId);
}

WP::err EncryptedUserData::readSafe(const QString &path, QByteArray &data) const
{
    return readSafe(path, data, fDefaultKeyId);
}

WP::err EncryptedUserData::writeSafe(const QString &path, const QString &data)
{
    return writeSafe(path, data, fDefaultKeyId);
}

WP::err EncryptedUserData::readSafe(const QString &path, QString &data) const
{
    return readSafe(path, data, fDefaultKeyId);
}

WP::err EncryptedUserData::writeSafe(const QString &path, const QString &data, const QString &keyId)
{
    QByteArray arrayData = data.toLatin1();
    return writeSafe(path, arrayData, keyId);
}

WP::err EncryptedUserData::writeSafe(const QString &path, const QByteArray &data, const QString &keyId)
{
    SecureArray key;
    QByteArray iv;
    WP::err error = fKeyStore->readSymmetricKey(keyId, key, iv);
    if (error != WP::kOk)
        return error;

    QByteArray encrypted;
    error = fCrypto->encryptSymmetric(data, encrypted, key, iv);
    if (error != WP::kOk)
        return error;
    return write(path, encrypted);
}

WP::err EncryptedUserData::readSafe(const QString &path, QString &data, const QString &keyId) const
{
    QByteArray arrayData;
    WP::err error = readSafe(path, arrayData, keyId);
    data = arrayData;
    return error;
}

WP::err EncryptedUserData::readSafe(const QString &path, QByteArray &data, const QString &keyId) const
{
    SecureArray key;
    QByteArray iv;
    WP::err error = fKeyStore->readSymmetricKey(keyId, key, iv);
    if (error != WP::kOk)
        return error;

    QByteArray encrypted;
    error = read(path, encrypted);
    if (error != WP::kOk)
        return error;
    return fCrypto->decryptSymmetric(encrypted, data, key, iv);
}

WP::err EncryptedUserData::create(const QString &uid, KeyStore *keyStore, const QString defaultKeyId,
                                  bool addUidToBaseDir)
{
    setUid(uid);
    setKeyStore(keyStore);
    setDefaultKeyId(defaultKeyId);

    if (addUidToBaseDir) {
        QString newBaseDir;
        (fDatabaseBaseDir == "") ? newBaseDir = uid : newBaseDir = fDatabaseBaseDir + "/" + uid;
        setBaseDir(newBaseDir);
    }

    return writeConfig();
}

void EncryptedUserData::setKeyStore(KeyStore *keyStore)
{
    fKeyStore = keyStore;
}

QString EncryptedUserData::getDefaultKeyId() const
{
    return fDefaultKeyId;
}

void EncryptedUserData::setDefaultKeyId(const QString &keyId)
{
    fDefaultKeyId = keyId;
}


StorageDirectory::StorageDirectory(EncryptedUserData *database, const QString &directory) :
    fDatabase(database),
    fDirectory(directory)
{
}

void StorageDirectory::setTo(EncryptedUserData *database, const QString &directory)
{
    fDatabase = database;
    fDirectory = directory;
}

void StorageDirectory::setTo(StorageDirectory *storageDir)
{
    fDatabase = storageDir->fDatabase;
    fDirectory = storageDir->fDirectory;
}

WP::err StorageDirectory::write(const QString &path, const QByteArray &data)
{
    return fDatabase->write(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::write(const QString &path, const QString &data)
{
    return fDatabase->write(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::read(const QString &path, QByteArray &data) const
{
    return fDatabase->read(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::read(const QString &path, QString &data) const
{
    return fDatabase->read(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::writeSafe(const QString &path, const QString &data)
{
    return fDatabase->writeSafe(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::writeSafe(const QString &path, const QByteArray &data)
{
    return fDatabase->writeSafe(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::writeSafe(const QString &path, const QByteArray &data, const QString &keyId)
{
    return fDatabase->writeSafe(fDirectory + "/" + path, data, keyId);
}

WP::err StorageDirectory::readSafe(const QString &path, QString &data) const
{
    return fDatabase->readSafe(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::readSafe(const QString &path, QByteArray &data, const QString &keyId) const
{
    return fDatabase->readSafe(fDirectory + "/" + path, data, keyId);
}

WP::err StorageDirectory::readSafe(const QString &path, QByteArray &data) const
{
    return fDatabase->readSafe(fDirectory + "/" + path, data);
}

WP::err StorageDirectory::remove(const QString &path)
{
    return fDatabase->getDatabase()->remove(fDirectory + "/" + path);
}

const QString &StorageDirectory::getDirectory()
{
    return fDirectory;
}

void StorageDirectory::setDirectory(const QString &directory)
{
    fDirectory = directory;
}

QStringList StorageDirectory::listDirectories(const QString &path) const
{
    return fDatabase->listDirectories(fDirectory + "/" + path);
}


DatabaseBranch::DatabaseBranch(const QString &database, const QString &branch) :
    fDatabasePath(database),
    fBranch(branch)
{
    DatabaseFactory::open(database, branch, &fDatabase);
}

DatabaseBranch::~DatabaseBranch()
{
    delete fDatabase;
}

void DatabaseBranch::setTo(const QString &database, const QString &branch)
{
    fDatabasePath = database;
    fBranch = branch;
}

const QString &DatabaseBranch::getDatabasePath() const
{
    return fDatabasePath;
}

const QString &DatabaseBranch::getBranch() const
{
    return fBranch;
}

QString DatabaseBranch::databaseHash() const
{
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    QByteArray hashResult = crypto->sha1Hash(fDatabasePath.toLatin1());
    return crypto->toHex(hashResult);
}

DatabaseInterface *DatabaseBranch::getDatabase() const
{
    return fDatabase;
}

WP::err DatabaseBranch::commit()
{
    if (fDatabase == NULL)
        return WP::kNotInit;
    return fDatabase->commit();
}

int DatabaseBranch::countRemotes() const
{
    return fRemotes.count();
}

RemoteDataStorage *DatabaseBranch::getRemoteAt(int i) const
{
    return fRemotes.at(i);
}

RemoteConnection *DatabaseBranch::getRemoteConnectionAt(int i) const
{
    return fRemotes.at(i)->getRemoteConnection();
}

RemoteAuthentication *DatabaseBranch::getRemoteAuthAt(int i) const
{
    return fRemotes.at(i)->getRemoteAuthentication();
}

WP::err DatabaseBranch::addRemote(RemoteDataStorage *data)
{
    fRemotes.append(data);
    return WP::kOk;
}
