#include "useridentity.h"

#include <QFile>
#include <QStringList>

#include "cryptointerface.h"

const char* kPathMailboxId = "mailbox_id";
const char* kPathIdentityKeyId = "identity_key_id";


UserIdentity::UserIdentity(DatabaseBranch *branch, const QString &baseDir) :
    mailbox(NULL),
    myselfContact(NULL),
    contactFinder(contactList)
{
    setToDatabase(branch, baseDir);
}

UserIdentity::~UserIdentity()
{
    // myself is in the contact list so don't delete
    foreach (Contact *contact, contactList)
        delete contact;
}


WP::err UserIdentity::createNewIdentity(KeyStore *keyStore, const QString &defaultKeyId,
                                        Mailbox *mailbox, bool addUidToBaseDir)
{
    // derive uid
    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = crypto->generateKeyPair(certificate, publicKey, privateKey, "");
    if (error != WP::kOk)
        return error;
    QByteArray hashResult = crypto->sha1Hash(certificate.toLatin1());
    QString uidHex = crypto->toHex(hashResult);

    // start creating the identity
    error = EncryptedUserData::create(uidHex, keyStore, defaultKeyId, addUidToBaseDir);
    if (error != WP::kOk)
        return error;
    mailbox = mailbox;

    QString keyId;
    error = keyStore->writeAsymmetricKey(certificate, publicKey, privateKey, keyId);
    if (error != WP::kOk)
        return error;

    myselfContact = new Contact(this, "myself");
    error = myselfContact->createUserIdentityContact(keyStore, keyId);
    if (error != WP::kOk)
        return error;

    error = write(kPathMailboxId, mailbox->getUid());
    if (error != WP::kOk)
        return error;

    QString outPut("signature.pup");
    writePublicSignature(outPut, publicKey);

    return error;
}

WP::err UserIdentity::open(KeyStoreFinder *keyStoreFinder, MailboxFinder *mailboxFinder)
{
    WP::err error = EncryptedUserData::open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    QString mailboxId;
    error = read(kPathMailboxId, mailboxId);
    if (error != WP::kOk)
        return error;

    myselfContact = new Contact(this, "myself");
    error = myselfContact->open(keyStoreFinder);
    if (error != WP::kOk)
        return error;
    contactList.append(myselfContact);

    QStringList contactNames = listDirectories("contacts");
    foreach (const QString &contactName, contactNames) {
        QString path = "contacts/" + contactName;
        Contact *contact = new Contact(this, path);
        WP::err error = contact->open(keyStoreFinder);
        if (error != WP::kOk) {
            delete contact;
            continue;
        }
        contactList.append(contact);
    }

    mailbox = mailboxFinder->find(mailboxId);
        if (mailbox == NULL)
            return WP::kEntryNotFound;
    mailbox->setOwner(this);
    return error;
}

Mailbox *UserIdentity::getMailbox() const
{
    return mailbox;
}

Contact *UserIdentity::getMyself() const
{
    return myselfContact;
}

const QList<Contact *> &UserIdentity::getContacts()
{
    return contactList;
}

WP::err UserIdentity::addContact(Contact *contact)
{
    QString contactUid = contact->getUid();
    QString path = "contacts/" + contactUid;
    contact->setTo(this, path);
    WP::err error = contact->writeConfig();
    if (error != WP::kOk)
        return error;
    contactList.append(contact);
    return WP::kOk;
}

Contact *UserIdentity::findContact(const QString &address)
{
    foreach (Contact *contact, contactList) {
        if (contact->getAddress() == address)
            return contact;
    }
    return NULL;
}

Contact *UserIdentity::findContactByUid(const QString &uid)
{
    return contactFinder.find(uid);
}

ContactFinder *UserIdentity::getContactFinder()
{
    return &contactFinder;
}

WP::err UserIdentity::writePublicSignature(const QString &filename, const QString &publicKey)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QByteArray data = publicKey.toLatin1();
    int count = data.count();
    if (file.write(data, count) != count)
        return WP::kError;
    return WP::kOk;
}


UserIdentity::UserIdContactFinder::UserIdContactFinder(QList<Contact *> &contactList) :
    contacts(contactList)
{

}

Contact *UserIdentity::UserIdContactFinder::find(const QString &uid)
{
    foreach (Contact *contact, contacts) {
        if (contact->getUid() == uid)
            return contact;
    }
    return NULL;
}
