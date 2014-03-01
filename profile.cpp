#include "profile.h"

#include <QStringList>

#include "mailbox.h"
#include "useridentity.h"
#include "remotestorage.h"


const char *kMainMailboxPath = "main_mailbox";


IdentityListModel::IdentityListModel(QObject * parent)
    :
      QAbstractListModel(parent)
{
}

QVariant IdentityListModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
        return identityList.at(index.row())->getUserData()->getUid();

    return QVariant();
}

int IdentityListModel::rowCount(const QModelIndex &/*parent*/) const
{
    return identityList.count();
}

void IdentityListModel::addIdentity(IdentityRef *identity)
{
    int count = identityList.count();
    beginInsertRows(QModelIndex(), count, count);
    identityList.append(identity);
    endInsertRows();
}

void IdentityListModel::removeIdentity(IdentityRef *identity)
{
    int index = identityList.indexOf(identity);
    beginRemoveRows(QModelIndex(), index, index);
    identityList.removeAt(index);
    endRemoveRows();
}

IdentityRef *IdentityListModel::removeIdentityAt(int index)
{
    IdentityRef *entry = identityList.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    identityList.removeAt(index);
    endRemoveRows();
    return entry;
}

UserIdentity *IdentityListModel::identityAt(int index)
{
    return identityList.at(index)->getUserData();
}

Profile::Profile(const QString &path, const QString &branchName)
{
    DatabaseBranch *databaseBranch = databaseBranchFor(path, branchName);
    setToDatabase(databaseBranch, "");
}

Profile::~Profile()
{
    clear();
}

WP::err Profile::createNewProfile(const SecureArray &password)
{
    setUid(crypto->generateUid());

    // init key store and master key
    DatabaseBranch *keyStoreBranch = databaseBranchFor(databaseBranch->getDatabasePath(), "key_stores");
    KeyStore* keyStore = NULL;
    WP::err error = createNewKeyStore(password, keyStoreBranch, &keyStore);
    if (error != WP::kOk)
        return error;
    setKeyStore(keyStore);

    SecureArray masterKey = crypto->generateSymmetricKey(256);
    QString masterKeyId;
    error = keyStore->writeSymmetricKey(masterKey, crypto->generateInitalizationVector(256),
                                        masterKeyId);
    if (error != WP::kOk)
        return error;

    setDefaultKeyId(masterKeyId);

    // create mailbox
    DatabaseBranch *mailboxesBranch = databaseBranchFor(databaseBranch->getDatabasePath(), "mailboxes");
    Mailbox *mailbox = NULL;
    error = createNewMailbox(mailboxesBranch, &mailbox);
    if (error != WP::kOk)
        return error;
    mainMailbox = mailbox->getUid();

    // init user identity
    DatabaseBranch *identitiesBranch = databaseBranchFor(databaseBranch->getDatabasePath(), "identities");
    UserIdentity *identity = NULL;
    error = createNewUserIdentity(identitiesBranch, mailbox, &identity);
    if (error != WP::kOk)
        return error;

    error = writeConfig();
    if (error != WP::kOk)
        return error;

    return WP::kOk;
}

WP::err Profile::writeConfig()
{
    WP::err error = EncryptedUserData::writeConfig();
    if (error != WP::kOk)
        return error;

    error = write(kMainMailboxPath, mainMailbox);
    return error;
}

WP::err Profile::commit()
{
    // just commit all branches
    for (int i = 0; i < branchList.count(); i++) {
        DatabaseBranch *branch = branchList.at(i);
        WP::err error = branch->commit();
        if (error != WP::kOk)
            return error;
    }
    return WP::kOk;
}

WP::err Profile::open(const SecureArray &password)
{
    if (database == NULL)
        return WP::kNotInit;

    WP::err error = loadKeyStores();
    if (error != WP::kOk)
        return error;

    error = read(kMainMailboxPath, mainMailbox);
    if (error != WP::kOk)
        return error;

    Profile::ProfileKeyStoreFinder keyStoreFinder(mapOfKeyStores);
    error = EncryptedUserData::open(&keyStoreFinder);
    if (error != WP::kOk)
        return error;
    error = keyStore->open(password);
    if (error != WP::kOk)
        return error;

    // remotes
    error = loadRemotes();
    if (error != WP::kOk)
        return error;

    // load database branches
    error = loadDatabaseBranches();
    if (error != WP::kOk)
        return error;

    // load mailboxes
    error = loadMailboxes();
    if (error != WP::kOk)
        return error;

    // load identities
    error = loadUserIdentities();
    if (error != WP::kOk)
        return error;

    return WP::kOk;
}

WP::err Profile::createNewKeyStore(const SecureArray &password, DatabaseBranch *branch, KeyStore **keyStoreOut)
{
    // init key store and master key
    KeyStore* keyStore = new KeyStore(branch, "");
    WP::err error = keyStore->create(password);
    if (error != WP::kOk)
        return error;
    error = addKeyStore(keyStore);
    if (error != WP::kOk)
        return error;
    *keyStoreOut = keyStore;
    return WP::kOk;
}

WP::err Profile::loadKeyStores()
{
    QStringList keyStores = listDirectories("key_stores");

    for (int i = 0; i < keyStores.count(); i++) {
        KeyStoreRef *entry
            = new KeyStoreRef(this, prependBaseDir("key_stores/" + keyStores.at(i)), NULL);
        if (entry->load(this) != WP::kOk)
            continue;
        addKeyStore(entry);
    }
    return WP::kOk;
}

WP::err Profile::addKeyStore(KeyStore *keyStore)
{
    QString dir = "key_stores/";
    dir += keyStore->getUid();

    KeyStoreRef *entry = new KeyStoreRef(this, dir, keyStore);
    WP::err error = entry->writeEntry();
    if (error != WP::kOk)
        return error;
    addKeyStore(entry);
    return WP::kOk;
}

void Profile::addKeyStore(KeyStoreRef *entry)
{
    mapOfKeyStores[entry->getUserData()->getUid()] = entry;
}

WP::err Profile::createNewUserIdentity(DatabaseBranch *branch, Mailbox *mailbox,
                                       UserIdentity **userIdentityOut)
{
    UserIdentity *identity = new UserIdentity(branch, "");
    WP::err error = identity->createNewIdentity(getKeyStore(), getDefaultKeyId(), mailbox);
    if (error != WP::kOk)
        return error;
    error = addUserIdentity(identity);
    if (error != WP::kOk)
        return error;
    mailbox->setOwner(identity);

    *userIdentityOut = identity;
    return WP::kOk;
}

WP::err Profile::loadUserIdentities()
{
    QStringList identityList = listDirectories("user_identities");

    for (int i = 0; i < identityList.count(); i++) {
        IdentityRef *entry
            = new IdentityRef(this, prependBaseDir("user_identities/" + identityList.at(i)), NULL);
        if (entry->load(this) != WP::kOk)
            continue;
        UserIdentity *id = entry->getUserData();
        Profile::ProfileKeyStoreFinder keyStoreFinder(mapOfKeyStores);
        Profile::ProfileMailboxFinder mailboxFinder(mapOfMailboxes);
        WP::err error = id->open(&keyStoreFinder, &mailboxFinder);
        if (error != WP::kOk){
            delete entry;
            continue;
        }

        addUserIdentity(entry);
    }
    return WP::kOk;
}

WP::err Profile::addUserIdentity(UserIdentity *identity)
{
    QString dir = "user_identities/";
    dir += identity->getUid();

    IdentityRef *entry = new IdentityRef(this, dir, identity);
    WP::err error = entry->writeEntry();
    if (error != WP::kOk)
        return error;
    addUserIdentity(entry);
    return WP::kOk;
}

Mailbox *Profile::getMailboxAt(int index) const
{
    QMap<QString, MailboxRef*>::const_iterator it  = mapOfMailboxes.begin();
    for (int i = 0; it != mapOfMailboxes.end() || i > index; i++) {
        if (i == index)
            return it.value()->getUserData();
    }
    return NULL;
}

void Profile::addUserIdentity(IdentityRef *entry)
{
    identitiesListModel.addIdentity(entry);
}

WP::err Profile::createNewMailbox(DatabaseBranch *branch, Mailbox **mailboxOut)
{
    Mailbox *mailbox = new Mailbox(branch, "");
    WP::err error = mailbox->createNewMailbox(getKeyStore(), getDefaultKeyId(), true);
    if (error != WP::kOk)
        return error;
    error = addMailbox(mailbox);
    if (error != WP::kOk)
        return error;
    *mailboxOut = mailbox;
    return WP::kOk;
}

WP::err Profile::loadMailboxes()
{
    QStringList mailboxList = listDirectories("mailboxes");

    for (int i = 0; i < mailboxList.count(); i++) {
        MailboxRef *entry
            = new MailboxRef(this, prependBaseDir("mailboxes/" + mailboxList.at(i)), NULL);
        if (entry->load(this) != WP::kOk)
            continue;
        Mailbox *mailbox = entry->getUserData();
        Profile::ProfileKeyStoreFinder keyStoreFinder(mapOfKeyStores);
        WP::err error = mailbox->open(&keyStoreFinder);
        if (error != WP::kOk){
            delete entry;
            continue;
        }

        addMailbox(entry);
    }
    return WP::kOk;
}

WP::err Profile::addMailbox(Mailbox *mailbox)
{
    QString dir = "mailboxes/";
    dir += mailbox->getUid();

    MailboxRef *entry = new MailboxRef(this, dir, mailbox);
    WP::err error = entry->writeEntry();
    if (error != WP::kOk)
        return error;
    addMailbox(entry);
    return WP::kOk;
}

void Profile::addMailbox(MailboxRef *entry)
{
    mapOfMailboxes[entry->getUserData()->getUid()] = entry;
}

WP::err Profile::loadRemotes()
{
    QStringList remotesList = listDirectories("remotes");

    for (int i = 0; i < remotesList.count(); i++) {
        RemoteDataStorage *remote
            = new RemoteDataStorage(this);
        StorageDirectory dir(this, prependBaseDir("remotes/" + remotesList.at(i)));
        if (remote->load(dir) != WP::kOk)
            continue;
        mapOfRemotes[remote->getUid()] = remote;
    }
    return WP::kOk;
}

RemoteDataStorage *Profile::findRemoteDataStorage(const QString &id)
{
    QMap<QString, RemoteDataStorage*>::iterator it = mapOfRemotes.find(id);
    if (it == mapOfRemotes.end())
        return NULL;
    return it.value();
}

WP::err Profile::writeDatabaseBranch(DatabaseBranch *databaseBranch)
{
    QString path = "remote_branches/";
    path += databaseBranch->databaseHash();

    WP::err error = writeSafe(path + "/path", databaseBranch->getDatabasePath());
    if (error != WP::kOk)
        return error;

    path += "/";
    path += databaseBranch->getBranch();
    path += "/remotes/";
    for (int i = 0; i < databaseBranch->countRemotes(); i++) {
        RemoteDataStorage *remote = databaseBranch->getRemoteAt(i);
        QString remotePath = path;
        remotePath += remote->getUid();
        remotePath += "/id";
        error = writeSafe(remotePath, remote->getUid());
        if (error != WP::kOk)
            return error;
    }
    return WP::kOk;
}

WP::err Profile::loadDatabaseBranches()
{
    QString path = "remote_branches";
    QStringList databases = listDirectories(path);
    for (int i = 0; i < databases.count(); i++) {
        QString subPath = path + "/" + databases.at(i);
        QString databasePath;
        WP::err error = readSafe(subPath + "/path", databasePath);
        if (error != WP::kOk)
            continue;
        QStringList branches = listDirectories(subPath);
        for (int a = 0; a < branches.count(); a++) {
            DatabaseBranch *databaseBranch = databaseBranchFor(databasePath, branches.at(a));

            QString remotePath = subPath + "/" + branches.at(a) + "/remotes";
            QStringList remotes = listDirectories(remotePath);
            for (int r = 0; r < remotes.count(); r++) {
                QString remoteId;
                error = readSafe(remotePath + "/" + remotes.at(r) + "/id", remoteId);
                if (error != WP::kOk)
                    continue;
                RemoteDataStorage *remote = findRemote(remoteId);
                if (remote == NULL)
                    continue;
                error = connectRemote(databaseBranch, remote);
                if (error != WP::kOk)
                    continue;
            }
        }
    }
    return WP::kOk;
}

IdentityListModel *Profile::getIdentityList()
{
    return &identitiesListModel;
}


QList<DatabaseBranch *> &Profile::getBranches()
{
    return branchList;
}

void Profile::clear()
{
    QMap<QString, KeyStoreRef*>::const_iterator it;
    for (it = mapOfKeyStores.begin(); it != mapOfKeyStores.end(); it++)
        delete it.value();
    mapOfKeyStores.clear();

    while (identitiesListModel.rowCount() > 0)
        delete identitiesListModel.removeIdentityAt(0);

    for (int i = 0; i < branchList.count(); i++)
        delete branchList.at(i);
    branchList.clear();
}

KeyStore *Profile::findKeyStore(const QString &keyStoreId)
{
    QMap<QString, KeyStoreRef*>::const_iterator it;
    it = mapOfKeyStores.find(keyStoreId);
    if (it == mapOfKeyStores.end())
        return NULL;
    return it.value()->getUserData();
}

DatabaseBranch *Profile::databaseBranchFor(const QString &database, const QString &branch)
{
    for (int i = 0; i < branchList.count(); i++) {
        DatabaseBranch *databaseBranch = branchList.at(i);
        if (databaseBranch->getDatabasePath() == database && databaseBranch->getBranch() == branch)
            return databaseBranch;
    }
    // not found create one
    DatabaseBranch *databaseBranch = new DatabaseBranch(database, branch);
    if (databaseBranch == NULL)
        return NULL;
    branchList.append(databaseBranch);
    return databaseBranch;
}

RemoteDataStorage *Profile::addPHPRemote(const QString &url)
{
    RemoteDataStorage *remoteDataStorage = new RemoteDataStorage(this);
    remoteDataStorage->setPHPEncryptedRemoteConnection(url);

    WP::err error = addRemoteDataStorage(remoteDataStorage);
    if (error != WP::kOk) {
        delete remoteDataStorage;
        return NULL;
    }
    return remoteDataStorage;
}

RemoteDataStorage *Profile::addHTTPRemote(const QString &url)
{
    RemoteDataStorage *remoteDataStorage = new RemoteDataStorage(this);
    remoteDataStorage->setHTTPRemoteConnection(url);

    WP::err error = addRemoteDataStorage(remoteDataStorage);
    if (error != WP::kOk) {
        delete remoteDataStorage;
        return NULL;
    }
    return remoteDataStorage;
}

WP::err Profile::setSignatureAuth(RemoteDataStorage *remote, const QString &userName,
                                  const QString &keyStoreId, const QString &keyId,
                                  const QString &serverName)
{
    RemoteDataStorage *inListRemote = findRemoteDataStorage(remote->getUid());
    if (inListRemote == NULL || inListRemote != remote)
        return WP::kEntryNotFound;

    remote->setSignatureAuth(userName, keyStoreId, keyId, serverName);
    StorageDirectory dir(this, prependBaseDir("remotes/" + remote->getUid()));
    return remote->write(dir);
}

WP::err Profile::addRemoteDataStorage(RemoteDataStorage *remote)
{
    // already exist?
    if (findRemoteDataStorage(remote->getUid()) !=  NULL)
        return WP::kEntryExist;

    StorageDirectory dir(this, prependBaseDir("remotes/" + remote->getUid()));
    WP::err error = remote->write(dir);
    if (error != WP::kOk)
        return error;
    mapOfRemotes[remote->getUid()] = remote;
    return WP::kOk;
}

WP::err Profile::connectRemote(DatabaseBranch *branch, RemoteDataStorage *remote)
{
    branch->addRemote(remote);
    return writeDatabaseBranch(branch);
}

RemoteDataStorage *Profile::findRemote(const QString &remoteId)
{
    QMap<QString, RemoteDataStorage*>::iterator it;
    it = mapOfRemotes.find(remoteId);
    if (it == mapOfRemotes.end())
        return NULL;
    return it.value();
}

WP::err Profile::connectFreeBranches(RemoteDataStorage *remote)
{
    // exist?
    if (findRemoteDataStorage(remote->getUid()) ==  NULL)
        return WP::kEntryNotFound;

    for (int i = 0; i < branchList.count(); i++) {
        DatabaseBranch *branch = branchList.at(i);
        connectRemote(branch, remote);
    }
    return WP::kOk;
}

KeyStore *Profile::ProfileKeyStoreFinder::find(const QString &keyStoreId)
{
    QMap<QString, KeyStoreRef*>::const_iterator it;
    it = mapOfKeyStores.find(keyStoreId);
    if (it == mapOfKeyStores.end())
        return NULL;
    return it.value()->getUserData();
}


IdentityRef::IdentityRef(EncryptedUserData *database, const QString &path,
                                           UserIdentity *identity) :
    UserDataRef<UserIdentity>(database, path, identity)
{
}

UserIdentity *IdentityRef::instanciate()
{
    return new UserIdentity(databaseBranch, databaseBaseDir);
}

KeyStoreRef::KeyStoreRef(EncryptedUserData *database, const QString &path,
                                           KeyStore *keyStore) :
    UserDataRef<KeyStore>(database, path, keyStore)
{
}

KeyStore *KeyStoreRef::instanciate()
{
    return new KeyStore(databaseBranch, databaseBaseDir);
}

Profile::ProfileKeyStoreFinder::ProfileKeyStoreFinder(QMap<QString, KeyStoreRef *> &map) :
    mapOfKeyStores(map)
{
}

MailboxRef::MailboxRef(EncryptedUserData *database, const QString &path, Mailbox *mailbox) :
    UserDataRef<Mailbox>(database, path, mailbox)
{

}

Mailbox *MailboxRef::instanciate()
{
    return new Mailbox(databaseBranch, databaseBaseDir);
}


Profile::ProfileMailboxFinder::ProfileMailboxFinder(QMap<QString, MailboxRef *> &map) :
    mapOfKeyMailboxes(map)
{
}

Mailbox *Profile::ProfileMailboxFinder::find(const QString &mailboxId)
{
    QMap<QString, MailboxRef*>::const_iterator it;
    it = mapOfKeyMailboxes.find(mailboxId);
    if (it == mapOfKeyMailboxes.end())
        return NULL;
    return it.value()->getUserData();
}
