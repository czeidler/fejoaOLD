#ifndef MESSAGETHREADDATAMODEL_H
#define MESSAGETHREADDATAMODEL_H

#include <QAbstractListModel>
#include <QList>

#include "mail.h"

class MessageListModel;


class MessageThread {
public:
    MessageThread(MessageChannel *channel);
    ~MessageThread();

    MessageChannelRef getMessageChannel() const;
    MessageListModel &getMessages() const;
    QVector<MessageChannelInfoRef> &getChannelInfos();

    MessageRef getLastMessage() const;
    void setLastMessage(MessageRef message);

private:
    MessageChannelRef channel;
    MessageListModel *messages;
    QVector<MessageChannelInfoRef> channelInfoList;
    MessageRef lastMessage;
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
