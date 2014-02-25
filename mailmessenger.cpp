#include "mailmessenger.h"

#include "protocolparser.h"
#include "remoteauthentication.h"
#include "useridentity.h"


MailMessenger::MailMessenger(Mailbox *mailbox, const MessageChannelInfo::Participant *_receiver, Profile *profile) :
    mailbox(mailbox),
    userIdentity(mailbox->getOwner()),
    receiver(_receiver),
    targetContact(NULL),
    contactRequest(NULL),
    message(NULL),
    deleteMessageWhenDone(false),
    remoteConnection(NULL)
{
    parseAddress(receiver->address);
    remoteConnection = ConnectionManager::defaultConnectionFor(QUrl(targetServer));
    if (remoteConnection == NULL)
        return;
    authentication = new SignatureAuthentication(remoteConnection, profile, userIdentity->getMyself()->getUid(),
                                                  userIdentity->getKeyStore()->getUid(),
                                                  userIdentity->getMyself()->getKeys()->getMainKeyId(), targetUser);
}

MailMessenger::~MailMessenger()
{
    delete authentication;
    delete remoteConnection;
}

WP::err MailMessenger::postMessage(Message *message, bool deleteWhenDone)
{
    if (this->message != NULL)
        return WP::kError;
    this->message = message;
    this->deleteMessageWhenDone = deleteWhenDone;

    if (targetServer == "")
        return WP::kNotInit;
    if (remoteConnection == NULL)
        return WP::kNotInit;
    if (authentication == NULL)
        return WP::kNotInit;

    Contact *contact = userIdentity->findContact(receiver->address);
    if (contact != NULL) {
        onContactFound(WP::kOk);
        return WP::kOk;
    } else if (receiver->uid != "") {
        contact = userIdentity->findContactByUid(receiver->uid);
        if (contact != NULL) {
            onContactFound(WP::kOk);
            return WP::kOk;
        }
    }

    return startContactRequest();
}

void MailMessenger::authConnected(WP::err error)
{
    if (error == WP::kContactNeeded) {
        startContactRequest();
        return;
    } else if (error != WP::kOk)
        return;

    MessageChannel *targetMessageChannel = NULL;
    if (((MessageChannel*)message->getChannel())->isNewLocale()) {
        targetMessageChannel = new MessageChannel((MessageChannel*)message->getChannel(), targetContact,
                                              targetContact->getKeys()->getMainKeyId());
    }

    QByteArray data;
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kSet);
    outStream.pushStanza(iqStanza);

    Contact *myself = userIdentity->getMyself();
    OutStanza *messageStanza =  new OutStanza("put_message");
    messageStanza->addAttribute("channel", message->getChannel()->getUid());
    outStream.pushChildStanza(messageStanza);

    QString signatureKeyId = myself->getKeys()->getMainKeyId();

    // write new channel
    if (targetMessageChannel != NULL) {
        error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, targetMessageChannel, "channel");
        if (error != WP::kOk) {
            emit sendResult(error);
            return;
        }
    }

    // write new channel info
    MessageChannelInfo *info = message->getChannelInfo();
    if (info->isNewLocale()) {
        info->setParticipantUid(targetContact->getAddress(), targetContact->getUid());
        error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, info, "channel_info");
        if (error != WP::kOk) {
            emit sendResult(error);
            return;
        }
    }

    // write message
    error = XMLSecureParcel::write(&outStream, myself, signatureKeyId, message, "message");
    if (error != WP::kOk) {
        emit sendResult(error);
        return;
    }

    outStream.flush();

    delete targetMessageChannel;

    if (deleteMessageWhenDone)
        delete message;
    message = NULL;

    serverReply = remoteConnection->send(data);
    connect(serverReply, SIGNAL(finished(WP::err)), this, SLOT(handleReply(WP::err)));
}

void MailMessenger::onContactFound(WP::err error)
{
    delete contactRequest;
    contactRequest = NULL;

    if (error != WP::kOk)
        return;

    targetContact = userIdentity->findContact(receiver->address);
    if (targetContact == NULL)
        return;

    if (authentication->isVerified())
        authConnected(WP::kOk);
    else {
        authentication->disconnect(this);
        connect(authentication, SIGNAL(authenticationAttemptFinished(WP::err)),
                this, SLOT(authConnected(WP::err)));
        authentication->login();
    }
}

void MailMessenger::handleReply(WP::err error)
{
    QByteArray data = serverReply->readAll();
    emit sendResult(error);
}

void MailMessenger::parseAddress(const QString &targetAddress)
{
    QString address = targetAddress.trimmed();
    QStringList parts = address.split("@");
    if (parts.count() != 2)
        return;
    targetUser = parts[0];
    targetServer = "http://";
    targetServer += parts[1];
    targetServer += "/php_server/portal.php";
}

WP::err MailMessenger::startContactRequest()
{
    contactRequest = new ContactRequest(remoteConnection, targetUser, userIdentity, this);
    connect(contactRequest, SIGNAL(contactRequestFinished(WP::err)), this, SLOT(onContactFound(WP::err)));
    return contactRequest->postRequest();
}


MultiMailMessenger::MultiMailMessenger(Mailbox *_mailbox, Profile *_profile) :
    mailMessenger(NULL),
    message(NULL),
    mailbox(_mailbox),
    messageChannelInfo(NULL),
    profile(_profile),
    lastParticipantIndex(-1)
{

}

MultiMailMessenger::~MultiMailMessenger()
{
    delete mailMessenger;
    delete message;
}

WP::err MultiMailMessenger::postMessage(Message *message)
{
    lastParticipantIndex = -1;

    delete this->message;
    this->message = message;

    // store message
    WP::err error = mailbox->storeMessage(message);
    if (error != WP::kOk)
        return error;

    messageChannelInfo = message->getChannelInfo();

    // TODO: try to get all participant uids first (if missing)

    onSendResult(WP::kOk);

    return WP::kOk;
}

void MultiMailMessenger::onSendResult(WP::err error)
{
    if (error != WP::kOk) {
        // todo
    }

    lastParticipantIndex++;
    if (lastParticipantIndex == messageChannelInfo->getParticipants().size()) {
        emit messagesSent();
        delete message;
        return;
    }
    const MessageChannelInfo::Participant *participant = &messageChannelInfo->getParticipants().at(lastParticipantIndex);
    Contact *myself = mailbox->getOwner()->getMyself();
    if (participant->uid == myself->getUid())
        onSendResult(WP::kOk);

    //mailMessenger->deleteLater();
    mailMessenger = new MailMessenger(mailbox, participant, profile);
    mailMessenger->postMessage(message, false);

    connect(mailMessenger, SIGNAL(sendResult(WP::err)), this, SLOT(onSendResult(WP::err)));
}
