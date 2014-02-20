#ifndef MESSAGETHREADDATAMODEL_H
#define MESSAGETHREADDATAMODEL_H

#include <QAbstractListModel>
#include <QList>

class Message;
class MessageChannel;
class MessageChannelInfo;
class MessageListModel;


class MessageThread {
public:
    MessageThread(MessageChannel *channel);
    ~MessageThread();

    MessageChannel *getMessageChannel() const;
    MessageListModel &getMessages() const;
    QList<MessageChannelInfo *> &getChannelInfos();

    Message *getLastMessage() const;
    void setLastMessage(Message *message);

private:
    MessageChannel *channel;
    MessageListModel *messages;
    QList<MessageChannelInfo*> channelInfoList;
    Message *lastMessage;
};

class MessageThreadDataModel : public QAbstractListModel {
public:
    MessageThreadDataModel(QObject * parent = 0);
    ~MessageThreadDataModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    int getChannelCount() const;
    void addChannel(MessageThread *channel);
    MessageThread *removeChannelAt(int index);
    bool removeChannel(MessageThread *channel);
    MessageThread *channelAt(int index) const;
    MessageThread *findChannel(const QString &channelId) const;

    void clear();

    void sort();

private:
    QList<MessageThread*> channels;
};

#endif // MESSAGETHREADDATAMODEL_H
