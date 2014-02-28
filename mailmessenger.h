#ifndef MAILMESSENGER_H
#define MAILMESSENGER_H

#include "contactrequest.h"
#include "mail.h"
#include "mailbox.h"
#include "remoteauthentication.h"

class Profile;
class UserIdentity;

class MailMessenger : public QObject {
Q_OBJECT
public:
    MailMessenger(Mailbox *mailbox, const MessageChannelInfo::Participant *receiver, Profile *profile);
    ~MailMessenger();

    WP::err postMessage(MessageRef message);

signals:
    void sendResult(WP::err error);

private slots:
    void handleReply(WP::err error);
    void authConnected(WP::err error);
    void onContactFound(WP::err error);

private:
    void parseAddress(const QString &targetAddress);
    WP::err startContactRequest();

    Mailbox *mailbox;

    UserIdentity *userIdentity;
    const MessageChannelInfo::Participant *receiver;
    QString targetServer;
    QString targetUser;

    Contact *targetContact;

    ContactRequest* contactRequest;

    MessageRef message;

    RemoteConnection *remoteConnection;
    RemoteConnectionReply *serverReply;
    RemoteAuthentication *authentication;
};


class MultiMailMessenger : public QObject {
Q_OBJECT
public:
    MultiMailMessenger(Mailbox *mailbox, Profile *profile);
    ~MultiMailMessenger();

    WP::err postMessage(MessageRef message);

signals:
    void messagesSent();

private slots:
    void onSendResult(WP::err error);

private:
    MailMessenger *mailMessenger;
    MessageRef message;

    Mailbox *mailbox;
    MessageChannelInfoRef messageChannelInfo;
    Profile *profile;

    int lastParticipantIndex;
};

#endif // MAILMESSENGER_H
