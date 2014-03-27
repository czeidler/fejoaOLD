#ifndef MAILBOX_H
#define MAILBOX_H

#include <QAbstractListModel>

#include "databaseutil.h"
#include "mail.h"
#include "messagethreaddatamodel.h"


class UserIdentity;

class MessageListModel : public QAbstractListModel {
public:
    MessageListModel(QObject * parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    int getMessageCount() const;
    void addMessage(MessageRef message);
    bool removeMessage(MessageRef message);
    MessageRef removeMessageAt(int index);
    MessageRef messageAt(int index);

    void clear();
private:
    QVector<MessageRef> messages;
};

class Mailbox : public EncryptedUserData, public DiffMonitorWatcher
{
Q_OBJECT
public:
    Mailbox(DatabaseBranch *branch, const QString &baseDir = "");
    ~Mailbox();

    WP::err createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId,
                             bool addUidToBaseDir = true);
    WP::err open(KeyStoreFinder *keyStoreFinder);

    WP::err storeMessage(MessageRef message);

    void setOwner(UserIdentity *owner);
    UserIdentity *getOwner() const;

    MessageListModel &getMessages();
    MessageThreadDataModel &getThreads();

    MessageThread *findMessageThread(const QString &channelId);

    virtual void onNewDiffs(const DatabaseDiff &diff);

signals:
    void databaseReadProgress(float progress);
    void databaseRead();

private:
    class MailboxMessageChannelFinder : public MessageChannelFinder {
    public:
        MailboxMessageChannelFinder(MessageThreadDataModel *threads);
        virtual MessageChannelRef findChannel(const QString &channelUid);
        virtual MessageChannelInfoRef findChannelInfo(const QString &channelUid,
                                                    const QString &channelInfoUid);

    private:
        MessageThreadDataModel *threads;
    };

    QStringList getChannelUids(QString path);
    QStringList getChannelUids();

    MessageChannel* readChannel(const QString &channelPath);

    WP::err readMailDatabase();
    WP::err readThread(const QString &channelPath);
    WP::err readThreadContent(const QString &channelPath, MessageThread *thread);
    WP::err readThreadInfo(const QString &infoPath, MessageThread *thread);
    WP::err readThreadMessage(const QString &messagePath, MessageThread *thread, MessageRef& message);
    void onNewMessageArrived(MessageThread *thread, MessageRef& message);

    WP::err storeMessage(MessageRef message, MessageChannelRef channel);
    WP::err storeChannel(MessageChannelRef channel);
    WP::err storeChannelInfo(MessageChannelRef channel, MessageChannelInfoRef info);

    QStringList getUidFilePaths(QString path);
    QStringList getUidDirPaths(QString path);
    QString pathForMessageId(const QString &channelId, const QString &messageId);
    QString pathForChannelId(const QString &channelId);
    QString dirForChannelId(const QString &channelId);
    QString makeUidPath(const QString &uid);
    WP::err writeParcel(const QString &path, DataParcel *parcel);

    UserIdentity *owner;

    MessageThreadDataModel threadList;
    MailboxMessageChannelFinder channelFinder;
};


class MailboxFinder {
public:
    virtual ~MailboxFinder() {}
    virtual Mailbox *find(const QString &mailboxId) = 0;
};

#endif // MAILBOX_H
