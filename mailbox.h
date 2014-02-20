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
    void addMessage(Message *message);
    bool removeMessage(Message *message);
    Message *removeMessageAt(int index);
    Message *messageAt(int index);

    void clear();
private:
    QList<Message*> messages;
};

class Mailbox : public EncryptedUserData
{
Q_OBJECT
public:
    Mailbox(DatabaseBranch *branch, const QString &baseDir = "");
    ~Mailbox();

    WP::err createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId,
                             bool addUidToBaseDir = true);
    WP::err open(KeyStoreFinder *keyStoreFinder);

    void setOwner(UserIdentity *userIdentity);
    UserIdentity *getOwner() const;

    MessageListModel &getMessages();
    MessageThreadDataModel &getThreads();

    MessageThread *findMessageThread(const QString &channelId);

signals:
    void databaseReadProgress(float progress);
    void databaseRead();

private slots:
    virtual void onNewCommits(const QString &startCommit, const QString &endCommit);

private:
    class MailboxMessageChannelFinder : public MessageChannelFinder {
    public:
        MailboxMessageChannelFinder(MessageThreadDataModel *threads);
        virtual MessageChannel *findChannel(const QString &channelUid);
        virtual MessageChannelInfo *findChannelInfo(const QString &channelUid,
                                                    const QString &channelInfoUid);

    private:
        MessageThreadDataModel *threads;
    };

    QStringList getChannelUids(QString path);
    QStringList getChannelUids();

    QString pathForChannelId(const QString &threadId);
    QString pathForMessagelId(const QString &threadPath, const QString &messageId);

    MessageChannel* readChannel(const QString &channelPath);

    WP::err readMailDatabase();

    UserIdentity *fUserIdentity;

    MessageThreadDataModel fThreadList;
    MailboxMessageChannelFinder channelFinder;
    WP::err readThreadContent(const QString &channelPath, MessageThread *thread);
    QStringList getUidFilePaths(QString path);
    QStringList getUidDirPaths(QString path);
};


class MailboxFinder {
public:
    virtual ~MailboxFinder() {}
    virtual Mailbox *find(const QString &mailboxId) = 0;
};

#endif // MAILBOX_H
