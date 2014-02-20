#include "messagethreaddatamodel.h"

#include "mailbox.h"


MessageThread::MessageThread(MessageChannel *channel) :
    channel(channel)
{
    messages = new MessageListModel();
}

MessageThread::~MessageThread()
{
    delete messages;
    delete channel;

    foreach (MessageChannelInfo *info, channelInfoList)
        delete info;
}

MessageChannel *MessageThread::getMessageChannel() const
{
    return channel;
}

MessageListModel &MessageThread::getMessages() const
{
    return *messages;
}

QList<MessageChannelInfo*> &MessageThread::getChannelInfos()
{
    return channelInfoList;
}

Message *MessageThread::getLastMessage() const
{
    return lastMessage;
}

void MessageThread::setLastMessage(Message *message)
{
    lastMessage = message;
}

MessageThreadDataModel::MessageThreadDataModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

QVariant MessageThreadDataModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    QString text;

    MessageThread* thread = channels.at(index.row());
    if (thread->getChannelInfos().size() > 0) {
        MessageChannelInfo *info = thread->getChannelInfos().at(0);
        text += info->getSubject();
        if (text == "")
            text += " ";
        if (info->getParticipants().size() > 0) {
            text += "(";
            QVector<MessageChannelInfo::Participant> &participants = info->getParticipants();
            for (int i = 0; i < participants.size(); i++) {
                text += participants.at(i).address;
                if (i < participants.size() - 1)
                    text += ",";
            }
            text += ")";
        }

    }

    if (text == "")
        text += thread->getMessageChannel()->getUid();

    return text;
}

int MessageThreadDataModel::rowCount(const QModelIndex &parent) const
{
    return getChannelCount();
}

int MessageThreadDataModel::getChannelCount() const
{
    return channels.count();
}

void MessageThreadDataModel::addChannel(MessageThread *channel)
{
    int index = channels.size();
    beginInsertRows(QModelIndex(), index, index);
    channels.append(channel);
    endInsertRows();
}

MessageThread *MessageThreadDataModel::removeChannelAt(int index)
{
    MessageThread *channel = channels.at(index);
    beginRemoveRows(QModelIndex(), index, index);
    channels.removeAt(index);
    endRemoveRows();
    return channel;
}

bool MessageThreadDataModel::removeChannel(MessageThread *channel)
{
    int index = channels.indexOf(channel);
    if (index < 0)
        return false;
    removeChannelAt(index);
    return true;
}

MessageThread *MessageThreadDataModel::channelAt(int index) const
{
    return channels.at(index);
}

MessageThread *MessageThreadDataModel::findChannel(const QString &channelId) const
{
    foreach (MessageThread *thread, channels) {
        if (thread->getMessageChannel()->getUid() == channelId)
            return thread;
    }
    return NULL;
}

void MessageThreadDataModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, channels.count() - 1);
    foreach (MessageThread *ref, channels)
        delete ref;
    channels.clear();
    endRemoveRows();
}

bool messageThreadComparator(const MessageThread *a, const MessageThread *b)
{
    if (a->getLastMessage() == NULL)
        return false;
    if (b->getLastMessage() == NULL)
        return true;
    return a->getLastMessage()->getTimestamp() > b->getLastMessage()->getTimestamp();
}

void MessageThreadDataModel::sort()
{
    beginResetModel();
    qSort(channels.begin(), channels.end(), messageThreadComparator);
    endResetModel();
}

MessageThreadDataModel::~MessageThreadDataModel()
{
    clear();
}
