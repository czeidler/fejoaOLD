#include "contact.h"


Contact::Contact(const QString &uid, const QString &keyId,
                 const QString &certificate, const QString &publicKey) :
    StorageDirectory(NULL, ""),
    privateKeyStore(false),
    uid(uid)
{
    keys = new ContactKeysBuddies(NULL, "");
    keys->addKeySet(keyId, certificate, publicKey);
    keys->setMainKeyId(keyId);
}

Contact::Contact(EncryptedUserData *database, const QString &directory) :
    StorageDirectory(database, directory),
    privateKeyStore(false),
    keys(NULL)
{

}

Contact::~Contact()
{
    delete keys;
}

WP::err Contact::createUserIdentityContact(KeyStore *keyStore, const QString &keyId)
{
    uid = keyId;

    privateKeyStore = true;

    keys = new ContactKeysKeyStore(database, directory + "/keys", keyStore);
    keys->addKeySet(keyId, true);

    return writeConfig();
}


WP::err Contact::open(KeyStoreFinder *keyStoreFinder)
{
    QString type;
    WP::err error = read("keystore_type", type);
    if (error == WP::kOk && type == "private") {
        privateKeyStore = true;
        keys = new ContactKeysKeyStore(database, getKeysDirectory(), database->getKeyStore());
    } else
        keys = new ContactKeysBuddies(database, getKeysDirectory());

    error = keys->open();
    if (error != WP::kOk)
        return error;

    error = read("uid", uid);
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
    return uid;
}

ContactKeys *Contact::getKeys()
{
    return keys;
}

const QString Contact::getAddress() const
{
    return serverUser + "@" + server;
}

const QString &Contact::getServerUser() const
{
    return serverUser;
}

const QString &Contact::getServer() const
{
    return server;
}

void Contact::setServerUser(const QString &serverUser)
{
    this->serverUser = serverUser;
}

void Contact::setServer(const QString &server)
{
    this->server = server;
}

WP::err Contact::writeConfig()
{
    WP::err error = WP::kOk;
    if (privateKeyStore) {
        privateKeyStore = true;
        error = write("keystore_type", QString("private"));
        if (error != WP::kOk)
            return error;
    }
    keys->setTo(database, getKeysDirectory());
    error = keys->writeConfig();
    if (error != WP::kOk)
        return error;

    error = write("uid", uid);
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
    return directory + "/keys";
}

ContactKeys::ContactKeys(EncryptedUserData *database, const QString &directory) :
    StorageDirectory(database, directory)
{

}

const QString ContactKeys::getMainKeyId() const
{
    return mainKeyId;
}

WP::err ContactKeys::setMainKeyId(const QString &keyId)
{
    this->mainKeyId = keyId;
}

WP::err ContactKeys::open()
{
    return read("main_key_id", mainKeyId);
}

WP::err ContactKeys::writeConfig()
{
    return write("main_key_id", mainKeyId);
}

KeyStore *ContactKeys::getKeyStore()
{
    return NULL;
}


ContactKeysKeyStore::ContactKeysKeyStore(EncryptedUserData *database, const QString &directory,
                                         KeyStore *keyStore) :
    ContactKeys(database, directory),
    keyStore(keyStore)
{

}

WP::err ContactKeysKeyStore::writeConfig()
{
    QString keyStoreId = database->getKeyStore()->getUid();
    WP::err error = WP::kOk;
    foreach (const QString &keyId, keyIdList) {
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

    keyIdList = listDirectories("");
    return error;
}

WP::err ContactKeysKeyStore::getKeySet(const QString &keyId, QString &certificate,
                                       QString &publicKey, QString &privateKey) const
{
    if (!keyIdList.contains(keyId))
        return WP::kEntryNotFound;
    return keyStore->readAsymmetricKey(keyId, certificate, publicKey, privateKey);
}

WP::err ContactKeysKeyStore::addKeySet(const QString &keyId, bool setAsMainKey)
{
    // first test if the keyId is valid
    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = keyStore->readAsymmetricKey(keyId, certificate, publicKey,
                                                                privateKey);
    if (error != WP::kOk)
        return error;
    keyIdList.append(keyId);
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
    return keyStore;
}


ContactKeysBuddies::ContactKeysBuddies(EncryptedUserData *database, const QString &directory) :
    ContactKeys(database, directory)
{
}

WP::err ContactKeysBuddies::writeConfig()
{
    WP::err error = WP::kOk;
    QMap<QString, PublicKeySet>::iterator it;
    for (it = keyMap.begin(); it != keyMap.end(); it++) {
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
        keyMap[keyId] = keySet;
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
    QMap<QString, PublicKeySet>::const_iterator it = keyMap.find(keyId);
    if (it == keyMap.end())
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

    keyMap[keyId] = keySet;
    return WP::kOk;
}
