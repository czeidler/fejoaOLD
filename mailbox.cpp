#include "mailbox.h"

#include <exception>

#include <QDateTime>
#include <QStringList>

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "remoteconnection.h"
#include "useridentity.h"


class GitIOException : public std::exception
{
  virtual const char* what() const throw()
  {
    return "git io error";
  }
} git_io_exception;


MessageListModel::MessageListModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

QVariant
MessageListModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    Message *message = messages.at(index.row());
    QDateTime time;
    time.setTime_t(message->getTimestamp());

    QString text;
    text += time.toString("dd.MM.yy hh:mm:ss");
    text += " (";
    text += message->getSender()->getAddress();
    text += "): ";
    text += message->getBody();

    return text;
}

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    return getMessageCount();
}

int MessageListModel::getMessageCount() const
{
    return messages.count();
}

void MessageListModel::addMessage(Message *messageRef)
{
    int index = 0;
    for (index; index < messages.count(); index++) {
        Message *current = messages.at(index);
        if (current->getTimestamp() > messageRef->getTimestamp())
            break;
    }

    beginInsertRows(QModelIndex(), index, index);
    messages.insert(index, messageRef);
    endInsertRows();
}

bool MessageListModel::removeMessage(Message *message)
{
    int index = messages.indexOf(message);
    if (index < 0)
        return false;
    removeMessageAt(index);
    return true;
}

Message *MessageListModel::removeMessageAt(int index)
{
    Message *entry = messages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    messages.removeAt(index);
    endRemoveRows();
    return entry;
}

Message *MessageListModel::messageAt(int index)
{
    return messages.at(index);
}

void MessageListModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, messages.count() - 1);
    foreach (Message *ref, messages)
        delete ref;
    messages.clear();
    endRemoveRows();
}


Mailbox::Mailbox(DatabaseBranch *branch, const QString &baseDir) :
    fUserIdentity(NULL),
    channelFinder(&fThreadList)
{
    setToDatabase(branch, baseDir);
}

Mailbox::~Mailbox()
{
}

WP::err Mailbox::createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId, bool addUidToBaseDir)
{
    // derive uid
    QString uidHex = fCrypto->generateUid();

    // start creating the identity
    WP::err error = EncryptedUserData::create(uidHex, keyStore, defaultKeyId, addUidToBaseDir);
    if (error != WP::kOk)
        return error;

    return error;
}

WP::err Mailbox::open(KeyStoreFinder *keyStoreFinder)
{
    WP::err error = EncryptedUserData::open(keyStoreFinder);
    if (error != WP::kOk)
        return error;

    return error;
}

WP::err Mailbox::storeMessage(Message *message)
{
    MessageChannel *channel = (MessageChannel*)message->getChannel();
    MessageChannelInfo *info = message->getChannelInfo();

    WP::err error = WP::kOk;
    if (channel->isNewLocale()) {
        Contact *myself = getOwner()->getMyself();
        MessageChannel *targetMessageChannel = new MessageChannel(channel, myself,
                                                                  myself->getKeys()->getMainKeyId());
        error = storeChannel(targetMessageChannel);
        delete targetMessageChannel;
        if (error != WP::kOk)
            return error;
    }

    if (info->isNewLocale()) {
        error = storeChannelInfo(channel, info);
        if (error != WP::kOk)
            return error;
    }

    return storeMessage(message, channel);
}

WP::err Mailbox::storeChannel(MessageChannel *channel)
{
    QString channelPath = pathForChannelId(channel->getUid());
    return writeParcel(channelPath, channel);
}

WP::err Mailbox::storeChannelInfo(MessageChannel *channel, MessageChannelInfo *info)
{
    QString channelInfoPath = dirForChannelId(channel->getUid());
    channelInfoPath += "/i/" + makeUidPath(info->getUid());
    return writeParcel(channelInfoPath, info);
}

WP::err Mailbox::storeMessage(Message *message, MessageChannel *channel)
{
    QString messagePath = pathForMessageId(channel->getUid(), message->getUid());
    messagePath += "/m";
    return writeParcel(messagePath, message);
}

QString Mailbox::pathForMessageId(const QString &channelId, const QString &messageId)
{
    return dirForChannelId(channelId) + "/" + makeUidPath(messageId);
}

QString Mailbox::pathForChannelId(const QString &channelId)
{
    return dirForChannelId(channelId) + "/r";
}

QString Mailbox::dirForChannelId(const QString &channelId)
{
    return makeUidPath(channelId);
}

QString Mailbox::makeUidPath(const QString &uid)
{
    QString path = uid.left(2);
    path += "/";
    path += uid.mid(2);
    return path;
}

WP::err Mailbox::writeParcel(const QString &path, DataParcel *parcel)
{
    Contact *myself = fUserIdentity->getMyself();

    QBuffer data;
    data.open(QBuffer::WriteOnly);
    WP::err error = parcel->toRawData(myself, myself->getKeys()->getMainKeyId(), data);
    if (error != WP::kOk)
        return error;
    return write(path, data.buffer());
}

void Mailbox::setOwner(UserIdentity *userIdentity)
{
    fUserIdentity = userIdentity;
    readMailDatabase();
    fThreadList.sort();
}

UserIdentity *Mailbox::getOwner() const
{
    return fUserIdentity;
}

MessageThreadDataModel &Mailbox::getThreads()
{
    return fThreadList;
}

QStringList Mailbox::getChannelUids(QString path)
{
    return getUidDirPaths(path);
}

QStringList Mailbox::getUidFilePaths(QString path)
{
    QStringList uidPaths;
    QStringList shortIds = listDirectories(path);
    for (int i = 0; i < shortIds.count(); i++) {
        const QString &shortId = shortIds.at(i);
        if (shortId.size() != 2)
            continue;
        QString shortPath = path;
        if (shortPath != "")
            shortPath += "/";
        shortPath += shortId;
        QStringList restIds = listFiles(shortPath);
        for (int a = 0; a < restIds.count(); a++)
            uidPaths.append(shortPath + "/" + restIds.at(a));

    }
    return uidPaths;
}

QStringList Mailbox::getUidDirPaths(QString path)
{
    QStringList uidPaths;
    QStringList shortIds = listDirectories(path);
    for (int i = 0; i < shortIds.count(); i++) {
        const QString &shortId = shortIds.at(i);
        if (shortId.size() != 2)
            continue;
        QString shortPath = path;
        if (shortPath != "")
            shortPath += "/";
        shortPath += shortId;
        QStringList restIds = listDirectories(shortPath);
        for (int a = 0; a < restIds.count(); a++)
            uidPaths.append(shortPath + "/" + restIds.at(a));

    }
    return uidPaths;
}

QStringList Mailbox::getChannelUids()
{
    return getChannelUids("");
}

MessageThread *Mailbox::findMessageThread(const QString &channelId)
{
    for (int i = 0; i < fThreadList.getChannelCount(); i++) {
        MessageThread *thread = fThreadList.channelAt(i);
        if (thread->getMessageChannel()->getUid() == channelId)
            return thread;
    }
    return NULL;
}

void Mailbox::onNewCommits(const QString &startCommit, const QString &endCommit)
{
    readMailDatabase();
}

WP::err Mailbox::readMailDatabase()
{
    fThreadList.clear();

    QStringList messageChannels = getChannelUids();
    foreach (const QString & channelPath, messageChannels) {
        MessageChannel* channel = readChannel(channelPath);
        if (channel == NULL) {
            delete channel;
            continue;
        }
        MessageThread *thread = new MessageThread(channel);
        fThreadList.addChannel(thread);
        WP::err error = readThreadContent(channelPath, thread);
        if (error != WP::kOk) {
            fThreadList.removeChannel(thread);
            delete thread;
        }

    }
    return WP::kOk;
}


WP::err Mailbox::readThreadContent(const QString &channelPath, MessageThread *thread)
{
    MessageListModel &messages = thread->getMessages();
    QList<MessageChannelInfo*> &infos = thread->getChannelInfos();

    QStringList infoUidPaths = getUidFilePaths(channelPath + "/i");
    for (int i = 0; i < infoUidPaths.count(); i++) {
        QString path = infoUidPaths.at(i);
        QByteArray data;
        WP::err error = read(path, data);
        if (error != WP::kOk)
            return error;

        // read channel info
        MessageChannelInfo *info = new MessageChannelInfo(&channelFinder);
        error = info->fromRawData(fUserIdentity->getContactFinder(), data);
        if (error == WP::kOk) {
            infos.append(info);
            continue;
        }
        delete info;
    }

    Message *lastMessage = NULL;

    QStringList messageUidPaths = getUidDirPaths(channelPath);
    for (int i = 0; i < messageUidPaths.count(); i++) {
        QString path = messageUidPaths.at(i);
        path += "/m";
        QByteArray data;
        WP::err error = read(path, data);
        if (error != WP::kOk)
            return error;

        // read message
        Message *message = new Message(&channelFinder);
        error = message->fromRawData(fUserIdentity->getContactFinder(), data);
        if (error == WP::kOk) {
            messages.addMessage(message);
            if (lastMessage == NULL || lastMessage->getTimestamp() < message->getTimestamp())
                lastMessage = message;
            continue;
        }
        delete message;
    }

    thread->setLastMessage(lastMessage);

    emit databaseRead();
    return WP::kOk;
}

MessageChannel* Mailbox::readChannel(const QString &channelPath) {
    QString path = channelPath;
    path += "/r";
    QByteArray data;
    WP::err error = read(path, data);
    if (error != WP::kOk)
        return NULL;

    Contact *myself = fUserIdentity->getMyself();
    MessageChannel *channel = new MessageChannel(myself);
    error = channel->fromRawData(fUserIdentity->getContactFinder(), data);
    if (error != WP::kOk) {
        delete channel;
        return NULL;
    }

    return channel;
}

Mailbox::MailboxMessageChannelFinder::MailboxMessageChannelFinder(MessageThreadDataModel *_threads) :
    threads(_threads)
{

}

MessageChannel *Mailbox::MailboxMessageChannelFinder::findChannel(const QString &channelUid)
{
    MessageThread *thread = threads->findChannel(channelUid);
    if (thread == NULL)
        return NULL;
    return thread->getMessageChannel();
}

MessageChannelInfo *Mailbox::MailboxMessageChannelFinder::findChannelInfo(const QString &channelUid,
                                                                          const QString &channelInfoUid)
{
    MessageThread *messageThread = NULL;
    for (int i = 0; i < threads->getChannelCount(); i++) {
        MessageThread *thread = threads->channelAt(i);
        if (thread->getMessageChannel()->getUid() == channelUid) {
            messageThread = thread;
            break;
        }
    }
    if (messageThread == NULL)
        return NULL;

    QList<MessageChannelInfo*>& channelInfos = messageThread->getChannelInfos();
    foreach (MessageChannelInfo* channelInfo, channelInfos) {
        if (channelInfo->getUid() == channelInfoUid)
            return channelInfo;
    }
    return NULL;
}
