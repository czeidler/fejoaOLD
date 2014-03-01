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

    MessageRef message = messages.at(index.row());
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

void MessageListModel::addMessage(MessageRef messageRef)
{
    int index = 0;
    for (; index < messages.count(); index++) {
        MessageRef current = messages.at(index);
        if (current->getTimestamp() > messageRef->getTimestamp())
            break;
    }

    beginInsertRows(QModelIndex(), index, index);
    messages.insert(index, messageRef);
    endInsertRows();
}

bool MessageListModel::removeMessage(MessageRef message)
{
    int index = messages.indexOf(message);
    if (index < 0)
        return false;
    removeMessageAt(index);
    return true;
}

MessageRef MessageListModel::removeMessageAt(int index)
{
    MessageRef entry = messages.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    messages.remove(index);
    endRemoveRows();
    return entry;
}

MessageRef MessageListModel::messageAt(int index)
{
    return messages[index];
}

void MessageListModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, messages.count() - 1);
    messages.clear();
    endRemoveRows();
}


Mailbox::Mailbox(DatabaseBranch *branch, const QString &baseDir) :
    owner(NULL),
    channelFinder(&threadList)
{
    setToDatabase(branch, baseDir);
}

Mailbox::~Mailbox()
{
}

WP::err Mailbox::createNewMailbox(KeyStore *keyStore, const QString &defaultKeyId, bool addUidToBaseDir)
{
    // derive uid
    QString uidHex = crypto->generateUid();

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

WP::err Mailbox::storeMessage(MessageRef message)
{
    MessageChannelRef channel = message->getChannel().staticCast<MessageChannel>();
    MessageChannelInfoRef info = message->getChannelInfo().staticCast<MessageChannelInfo>();

    WP::err error = WP::kOk;
    if (channel->isNewLocale()) {
        Contact *myself = getOwner()->getMyself();
        MessageChannelRef targetMessageChannel(new MessageChannel(channel, myself,
                                                                  myself->getKeys()->getMainKeyId()));
        error = storeChannel(targetMessageChannel);
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

WP::err Mailbox::storeChannel(MessageChannelRef channel)
{
    QString channelPath = pathForChannelId(channel->getUid());
    return writeParcel(channelPath, channel.data());
}

WP::err Mailbox::storeChannelInfo(MessageChannelRef channel, MessageChannelInfoRef info)
{
    Contact *myself = owner->getMyself();

    /* The info uid is calculated from its data when calling toRawData. However, the uid is
     * needed to determine the message path. For that reason we don't use writeParcel here.
     */
    QBuffer data;
    data.open(QBuffer::WriteOnly);
    WP::err error = info->toRawData(myself, myself->getKeys()->getMainKeyId(), data);
    if (error != WP::kOk)
        return error;

    QString channelInfoPath = dirForChannelId(channel->getUid());
    channelInfoPath += "/i/" + makeUidPath(info->getUid());
    return write(channelInfoPath, data.buffer());
}

WP::err Mailbox::storeMessage(MessageRef message, MessageChannelRef channel)
{
    Contact *myself = owner->getMyself();

    /* The message uid is calculated from its data when calling toRawData. However, the uid is
     * needed to determine the message path. For that reason we don't use writeParcel here.
     */
    QBuffer data;
    data.open(QBuffer::WriteOnly);
    WP::err error = message->toRawData(myself, myself->getKeys()->getMainKeyId(), data);
    if (error != WP::kOk)
        return error;

    QString messagePath = pathForMessageId(channel->getUid(), message->getUid());
    messagePath += "/m";

    return write(messagePath, data.buffer());
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
    Contact *myself = owner->getMyself();

    QBuffer data;
    data.open(QBuffer::WriteOnly);
    WP::err error = parcel->toRawData(myself, myself->getKeys()->getMainKeyId(), data);
    if (error != WP::kOk)
        return error;
    return write(path, data.buffer());
}

void Mailbox::setOwner(UserIdentity *userIdentity)
{
    this->owner = userIdentity;
    readMailDatabase();
    threadList.sort();
}

UserIdentity *Mailbox::getOwner() const
{
    return owner;
}

MessageThreadDataModel &Mailbox::getThreads()
{
    return threadList;
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
    for (int i = 0; i < threadList.getChannelCount(); i++) {
        MessageThread *thread = threadList.channelAt(i);
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
    threadList.clear();

    QStringList messageChannels = getChannelUids();
    foreach (const QString & channelPath, messageChannels) {
        MessageChannel* channel = readChannel(channelPath);
        if (channel == NULL) {
            delete channel;
            continue;
        }
        MessageThread *thread = new MessageThread(channel);
        threadList.addChannel(thread);
        WP::err error = readThreadContent(channelPath, thread);
        if (error != WP::kOk) {
            threadList.removeChannel(thread);
            delete thread;
        }

    }
    return WP::kOk;
}


WP::err Mailbox::readThreadContent(const QString &channelPath, MessageThread *thread)
{
    MessageListModel &messages = thread->getMessages();
    QVector<MessageChannelInfoRef> &infos = thread->getChannelInfos();

    QStringList infoUidPaths = getUidFilePaths(channelPath + "/i");
    for (int i = 0; i < infoUidPaths.count(); i++) {
        QString path = infoUidPaths.at(i);
        QByteArray data;
        WP::err error = read(path, data);
        if (error != WP::kOk)
            return error;

        // read channel info
        MessageChannelInfoRef info(new MessageChannelInfo(&channelFinder));
        error = info->fromRawData(owner->getContactFinder(), data);
        if (error == WP::kOk) {
            infos.append(info);
            continue;
        }
    }

    MessageRef lastMessage;

    QStringList messageUidPaths = getUidDirPaths(channelPath);
    for (int i = 0; i < messageUidPaths.count(); i++) {
        QString path = messageUidPaths.at(i);
        path += "/m";
        QByteArray data;
        WP::err error = read(path, data);
        if (error != WP::kOk)
            return error;

        // read message
        MessageRef message(new Message(&channelFinder));
        error = message->fromRawData(owner->getContactFinder(), data);
        if (error == WP::kOk) {
            messages.addMessage(message);
            if (lastMessage == NULL || lastMessage->getTimestamp() < message->getTimestamp())
                lastMessage = message;
            continue;
        }
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

    Contact *myself = owner->getMyself();
    MessageChannel *channel = new MessageChannel(myself);
    error = channel->fromRawData(owner->getContactFinder(), data);
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

MessageChannelRef Mailbox::MailboxMessageChannelFinder::findChannel(const QString &channelUid)
{
    MessageThread *thread = threads->findChannel(channelUid);
    if (thread == NULL)
        return MessageChannelRef();
    return thread->getMessageChannel();
}

MessageChannelInfoRef Mailbox::MailboxMessageChannelFinder::findChannelInfo(const QString &channelUid,
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
        return MessageChannelInfoRef();

    QVector<MessageChannelInfoRef> &channelInfos = messageThread->getChannelInfos();
    foreach (MessageChannelInfoRef channelInfo, channelInfos) {
        if (channelInfo->getUid() == channelInfoUid)
            return channelInfo;
    }
    return MessageChannelInfoRef();
}
