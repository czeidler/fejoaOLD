#ifndef PROFILE_H
#define PROFILE_H

#include <QAbstractListModel>
#include <QList>

#include "cryptointerface.h"
#include "databaseinterface.h"
#include "databaseutil.h"
#include "error_codes.h"
#include "mailbox.h"


class KeyStoreRef;
class MailboxRef;
class IdentityRef;
class RemoteConnection;
class UserIdentity;


class IdentityListModel : public QAbstractListModel {
public:
    IdentityListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    void addIdentity(IdentityRef *identity);
    void removeIdentity(IdentityRef *identity);
    IdentityRef *removeIdentityAt(int index);
    UserIdentity *identityAt(int index);
private:
    QList<IdentityRef*> fIdentities;
};


class Profile : public EncryptedUserData
{
public:
    Profile(const QString &path, const QString &branchName);
    ~Profile();

    /*! create a new profile and adds a new KeyStore with \par password. */
    WP::err createNewProfile(const SecureArray &password);

    WP::err writeConfig();

    WP::err commit();

    //! adds an external key store to the profile
    WP::err addKeyStore(KeyStore *keyStore);
    WP::err removeKeyStore(KeyStore *keyStore);
    KeyStore *findKeyStore(const QString &keyStoreId);

    WP::err addUserIdentity(UserIdentity *identity);
    WP::err removeUserIdentity(UserIdentity *useIdentity);

    int getMailboxCount() const;
    Mailbox * getMailboxAt(int index) const;

    DatabaseBranch* databaseBranchFor(const QString &database, const QString &branch);

    RemoteDataStorage *addPHPRemote(const QString &url);
    RemoteDataStorage *addHTTPRemote(const QString &url);
    WP::err setSignatureAuth(RemoteDataStorage *remote, const QString &userName,
                             const QString &keyStoreId, const QString &keyId,
                             const QString &serverName);
    WP::err connectRemote(DatabaseBranch *branch, RemoteDataStorage *remote);
    RemoteDataStorage *findRemote(const QString &remoteId);
    WP::err connectFreeBranches(RemoteDataStorage *remote);

    WP::err open(const SecureArray &password);

    IdentityListModel* getIdentityList();

    EncryptedUserData* findStorageDirectory(UserData *userData);

    QList<DatabaseBranch*> &getBranches();

private:
    void clear();

    WP::err createNewKeyStore(const SecureArray &password, DatabaseBranch *branch,
                              KeyStore **keyStoreOut);
    WP::err loadKeyStores();
    void addKeyStore(KeyStoreRef *entry);

    WP::err createNewUserIdentity(DatabaseBranch *branch, Mailbox *mailbox,
                                  UserIdentity **userIdentityOut);
    WP::err loadUserIdentities();
    void addUserIdentity(IdentityRef *entry);

    WP::err createNewMailbox(DatabaseBranch *branch, Mailbox **mailboxOut);
    WP::err loadMailboxes();
    WP::err addMailbox(Mailbox *mailbox);
    void addMailbox(MailboxRef *entry);

    WP::err loadRemotes();
    WP::err addRemoteDataStorage(RemoteDataStorage *remote);
    void addRemote(RemoteDataStorage *remote);
    RemoteDataStorage *findRemoteDataStorage(const QString &id);

    WP::err writeDatabaseBranch(DatabaseBranch *databaseBranch);
    WP::err loadDatabaseBranches();
private:
    class ProfileKeyStoreFinder : public KeyStoreFinder {
    public:
        ProfileKeyStoreFinder(QMap<QString, KeyStoreRef*> &map);
        virtual KeyStore *find(const QString &keyStoreId);
    private:
        QMap<QString, KeyStoreRef*> &fMapOfKeyStores;
    };

    class ProfileMailboxFinder : public MailboxFinder {
    public:
        ProfileMailboxFinder(QMap<QString, MailboxRef*> &map);
        virtual Mailbox *find(const QString &mailboxId);
    private:
        QMap<QString, MailboxRef*> &fMapOfKeyMailboxes;
    };

    IdentityListModel fIdentities;

    QMap<QString, KeyStoreRef*> fMapOfKeyStores;
    QString fMainMailbox;
    QMap<QString, MailboxRef*> fMapOfMailboxes;
    QMap<QString, RemoteDataStorage*> fMapOfRemotes;

    QList<DatabaseBranch*> fBranches;
};


template<class Type>
class UserDataRef : public StorageDirectory {
public:
    UserDataRef(EncryptedUserData *database, const QString &path, Type *userData) :
        StorageDirectory(database, path),
        fUserData(userData)
    {
        if (fUserData != NULL) {
            fDatabaseBranch = fUserData->getDatabaseBranch();
            fDatabaseBaseDir = fUserData->getDatabaseBaseDir();
        }
    }

    virtual ~UserDataRef() {
        delete fUserData;
    }

    virtual WP::err writeEntry()
    {
        WP::err error = write("database_path", fDatabaseBranch->getDatabasePath());
        if (error != WP::kOk)
            return error;
        error = write("database_branch", fDatabaseBranch->getBranch());
        if (error != WP::kOk)
            return error;
        error = write("database_base_dir", fDatabaseBaseDir);
        if (error != WP::kOk)
            return error;
        return WP::kOk;
    }

    virtual WP::err load(Profile *profile) {
        QString databasePath;
        WP::err error = read("database_path", databasePath);
        if (error != WP::kOk)
            return error;
        QString databaseBranch;
        error = read("database_branch", databaseBranch);
        if (error != WP::kOk)
            return error;
        fDatabaseBranch = profile->databaseBranchFor(databasePath, databaseBranch);
        error = read("database_base_dir", fDatabaseBaseDir);
        if (error != WP::kOk)
            return error;

        fUserData = instanciate();
        if (fUserData == NULL)
            return WP::kError;
        return WP::kOk;
    }

    void setUserData(Type *data) {
        delete fUserData;
        fUserData = data;
    }

    Type *getUserData() const {
        return fUserData;
    }

protected:
    virtual Type *instanciate() = 0;

protected:
    DatabaseBranch *fDatabaseBranch;
    QString fDatabaseBaseDir;

    Type *fUserData;
};


class KeyStore;

class KeyStoreRef : public UserDataRef<KeyStore> {
public:
    KeyStoreRef(EncryptedUserData *database, const QString &path,
                         KeyStore *keyStore);
protected:
    KeyStore *instanciate();
};


class UserIdentity;

class IdentityRef : public UserDataRef<UserIdentity> {
public:
    IdentityRef(EncryptedUserData *database, const QString &path, UserIdentity *identity);
protected:
    UserIdentity *instanciate();
};


class MailboxRef : public UserDataRef<Mailbox> {
public:
    MailboxRef(EncryptedUserData *database, const QString &path, Mailbox *mailbox);
protected:
    Mailbox *instanciate();
};


#endif // PROFILE_H
