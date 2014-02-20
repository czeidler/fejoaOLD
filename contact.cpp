#include "contact.h"


Contact::Contact(const QString &uid, const QString &keyId,
                 const QString &certificate, const QString &publicKey) :
    StorageDirectory(NULL, ""),
    fPrivateKeyStore(false)
{
    fUid = uid;

    fKeys = new ContactKeysBuddies(NULL, "");
    fKeys->addKeySet(keyId, certificate, publicKey);
    fKeys->setMainKeyId(keyId);
}

Contact::Contact(EncryptedUserData *database, const QString &directory) :
    StorageDirectory(database, directory),
    fPrivateKeyStore(false),
    fKeys(NULL)
{

}

Contact::~Contact()
{
    delete fKeys;
}

WP::err Contact::createUserIdentityContact(KeyStore *keyStore, const QString &keyId)
{
    fUid = keyId;

    fPrivateKeyStore = true;

    fKeys = new ContactKeysKeyStore(fDatabase, fDirectory + "/keys", keyStore);
    fKeys->addKeySet(keyId, true);

    return writeConfig();
}


WP::err Contact::open(KeyStoreFinder *keyStoreFinder)
{
    QString type;
    WP::err error = read("keystore_type", type);
    if (error == WP::kOk && type == "private") {
        fPrivateKeyStore = true;
        fKeys = new ContactKeysKeyStore(fDatabase, getKeysDirectory(), fDatabase->getKeyStore());
    } else
        fKeys = new ContactKeysBuddies(fDatabase, getKeysDirectory());

    error = fKeys->open();
    if (error != WP::kOk)
        return error;

    error = read("uid", fUid);
    if (error != WP::kOk)
        return error;

    QString address;
    error = read("address", address);
    if (error != WP::kOk)
        return error;

    setAddress(address);

    return error;
}

WP::err Contact::sign(const QString &keyId, const QByteArray &data, QByteArray &signature)
{
    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = getKeys()->getKeySet(keyId, certificate, publicKey, privateKey);
    if (error != WP::kOk)
        return error;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    SecureArray password = "";
    return crypto->sign(data, signature, privateKey, password);
}

bool Contact::verify(const QString &keyId, const QByteArray &data, const QByteArray &signature)
{
    QString certificate;
    QString publicKey;
    WP::err error = getKeys()->getKeySet(keyId, certificate, publicKey);
    if (error != WP::kOk)
        return false;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    return crypto->verifySignatur(data, signature, publicKey);
}

QString Contact::getUid() const
{
    return fUid;
}

ContactKeys *Contact::getKeys()
{
    return fKeys;
}

const QString Contact::getAddress() const
{
    return fServerUser + "@" + fServer;
}

const QString &Contact::getServerUser() const
{
    return fServerUser;
}

const QString &Contact::getServer() const
{
    return fServer;
}

void Contact::setServerUser(const QString &serverUser)
{
    fServerUser = serverUser;
}

void Contact::setServer(const QString &server)
{
    fServer = server;
}

WP::err Contact::writeConfig()
{
    WP::err error = WP::kOk;
    if (fPrivateKeyStore) {
        fPrivateKeyStore = true;
        error = write("keystore_type", QString("private"));
        if (error != WP::kOk)
            return error;
    }
    fKeys->setTo(fDatabase, getKeysDirectory());
    error = fKeys->writeConfig();
    if (error != WP::kOk)
        return error;

    error = write("uid", fUid);
    if (error != WP::kOk)
        return error;

    error = write("address", getAddress());
    if (error != WP::kOk)
        return error;

    return error;
}

bool Contact::setAddress(const QString &address)
{
    QStringList addressParts = address.split("@");
    if (addressParts.count() == 2) {
        setServerUser(addressParts[0]);
        setServer(addressParts[1]);
    } else {
        setServerUser("");
        setServer("");
    }

}

QString Contact::getKeysDirectory() const
{
    return fDirectory + "/keys";
}

ContactKeys::ContactKeys(EncryptedUserData *database, const QString &directory) :
    StorageDirectory(database, directory)
{

}

const QString ContactKeys::getMainKeyId() const
{
    return fMainKeyId;
}

WP::err ContactKeys::setMainKeyId(const QString &keyId)
{
    fMainKeyId = keyId;
}

WP::err ContactKeys::open()
{
    return read("main_key_id", fMainKeyId);
}

WP::err ContactKeys::writeConfig()
{
    return write("main_key_id", fMainKeyId);
}

KeyStore *ContactKeys::getKeyStore()
{
    return NULL;
}


ContactKeysKeyStore::ContactKeysKeyStore(EncryptedUserData *database, const QString &directory,
                                         KeyStore *keyStore) :
    ContactKeys(database, directory),
    fKeyStore(keyStore)
{

}

WP::err ContactKeysKeyStore::writeConfig()
{
    QString keyStoreId = fDatabase->getKeyStore()->getUid();
    WP::err error = WP::kOk;
    foreach (const QString &keyId, fKeyIdList) {
        error = write(keyId + "/keystore", keyStoreId);
        if (error != WP::kOk)
            return error;
    }

    return ContactKeys::writeConfig();
}

WP::err ContactKeysKeyStore::open()
{
    WP::err error = ContactKeys::open();
    if (error != WP::kOk)
        return error;

    fKeyIdList = listDirectories("");
    return error;
}

WP::err ContactKeysKeyStore::getKeySet(const QString &keyId, QString &certificate,
                                       QString &publicKey, QString &privateKey) const
{
    if (!fKeyIdList.contains(keyId))
        return WP::kEntryNotFound;
    return fKeyStore->readAsymmetricKey(keyId, certificate, publicKey, privateKey);
}

WP::err ContactKeysKeyStore::addKeySet(const QString &keyId, bool setAsMainKey)
{
    // first test if the keyId is valid
    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = fKeyStore->readAsymmetricKey(keyId, certificate, publicKey,
                                                                privateKey);
    if (error != WP::kOk)
        return error;
    fKeyIdList.append(keyId);
    if (setAsMainKey)
        setMainKeyId(keyId);
    return WP::kOk;
}

WP::err ContactKeysKeyStore::getKeySet(const QString &keyId, QString &certificate,
                                       QString &publicKey) const
{
    QString privateKey;
    return getKeySet(keyId, certificate, publicKey, privateKey);
}

WP::err ContactKeysKeyStore::addKeySet(const QString &keyId, const QString &certificate,
                                       const QString &publicKey)
{
    return WP::kError;
}

KeyStore *ContactKeysKeyStore::getKeyStore()
{
    return fKeyStore;
}


ContactKeysBuddies::ContactKeysBuddies(EncryptedUserData *database, const QString &directory) :
    ContactKeys(database, directory)
{
}

WP::err ContactKeysBuddies::writeConfig()
{
    WP::err error = WP::kOk;
    QMap<QString, PublicKeySet>::iterator it;
    for (it = fKeyMap.begin(); it != fKeyMap.end(); it++) {
        QString keyId = it.key();
        error = write(keyId + "/certificate", it.value().certificate);
        if (error != WP::kOk)
            return error;
        error = write(keyId + "/public_key", it.value().publicKey);
        if (error != WP::kOk)
            return error;
    }

    return ContactKeys::writeConfig();
}

WP::err ContactKeysBuddies::open()
{
    WP::err error = ContactKeys::open();
    if (error != WP::kOk)
        return error;

    QStringList keyIds = listDirectories("");
    foreach (const QString &keyId, keyIds) {
        PublicKeySet keySet;
        error = read(keyId + "/certificate", keySet.certificate);
        if (error != WP::kOk)
            return error;
        // the public key is needed by the server to verify incoming data so it can't be stored encrypted
        error = read(keyId + "/public_key", keySet.publicKey);
        if (error != WP::kOk)
            return error;
        fKeyMap[keyId] = keySet;
    }

    return error;
}

WP::err ContactKeysBuddies::getKeySet(const QString &keyId, QString &certificate,
                                      QString &publicKey, QString &privateKey) const
{
    return WP::kError;
}

WP::err ContactKeysBuddies::addKeySet(const QString &keyId, bool setAsMainKey)
{
    return WP::kError;
}

WP::err ContactKeysBuddies::getKeySet(const QString &keyId, QString &certificate,
                                      QString &publicKey) const
{
    QMap<QString, PublicKeySet>::const_iterator it = fKeyMap.find(keyId);
    if (it == fKeyMap.end())
        return WP::kEntryNotFound;

    certificate = it.value().certificate;
    publicKey = it.value().publicKey;
    return WP::kOk;
}

WP::err ContactKeysBuddies::addKeySet(const QString &keyId, const QString &certificate,
                                      const QString &publicKey)
{
    PublicKeySet keySet;
    keySet.certificate = certificate;
    keySet.publicKey = publicKey;

    fKeyMap[keyId] = keySet;
    return WP::kOk;
}
