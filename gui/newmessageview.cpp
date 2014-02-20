#include "newmessageview.h"

#include <QVBoxLayout>

#include "mailmessenger.h"
#include "profile.h"
#include "useridentity.h"

NewMessageView::NewMessageView(Profile *_profile, QWidget *parent) :
    QWidget(parent),
    profile(_profile)
{
    QVBoxLayout* composerLayout = new QVBoxLayout();
    setLayout(composerLayout);

    receiverEdit = new QLineEdit();
receiverEdit->setText("lec@localhost");
    subjectEdit = new QLineEdit();
    messageComposer = new QTextEdit();
    sendButton = new QPushButton("Send");
    composerLayout->addWidget(subjectEdit);
    composerLayout->addWidget(receiverEdit);
    composerLayout->addWidget(messageComposer);
    composerLayout->addWidget(sendButton);

    connect(sendButton, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
}

void NewMessageView::setMailbox(Mailbox *box)
{
    mailbox = box;
}

void NewMessageView::onSendButtonClicked()
{
    QString address = receiverEdit->text();
    if (address == "")
        return;

    QString subject = subjectEdit->text();
    MessageChannelInfo *channelInfo = new MessageChannelInfo((SecureChannel*)NULL);
    Contact *mySelf = mailbox->getOwner()->getMyself();

    channelInfo->addParticipant(mySelf->getAddress(), mySelf->getUid());
    if (mySelf->getAddress() != address)
        channelInfo->addParticipant(address, "");

    channelInfo->setSubject(subject);

    QString body = messageComposer->toPlainText();
    MultiMailMessenger *messenger = new MultiMailMessenger(mailbox, profile);

    Message *message = new Message(channelInfo);
    message->setBody(body.toLatin1());
    messenger->postMessage(message);

    // todo progress bar
    messageComposer->clear();
}
