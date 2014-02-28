#include "threadview.h"

#include <QVBoxLayout>

#include "mailbox.h"
#include "mailmessenger.h"
#include "profile.h"
#include "useridentity.h"


ThreadView::ThreadView(Profile *profile, QWidget *parent) :
    QSplitter(Qt::Vertical, parent),
    mailbox(NULL),
    profile(profile)
{
    QWidget *topWidget = new QWidget(this);
    QWidget *bottomWidget = new QWidget(this);
    setStretchFactor(0, 5);
    setStretchFactor(1, 2);

    // top
    QVBoxLayout* topLayout = new QVBoxLayout();
    topWidget->setLayout(topLayout);
    subjectLabel = new QLabel(topWidget);
    participantsLabel = new QLabel(topWidget);
    messageDisplay = new QListView(topWidget);
    topLayout->addWidget(subjectLabel);
    topLayout->addWidget(participantsLabel);
    topLayout->addWidget(messageDisplay);

    // bottom
    QVBoxLayout* composerLayout = new QVBoxLayout();
    bottomWidget->setLayout(composerLayout);

    messageComposer = new QTextEdit(bottomWidget);
    sendButton = new QPushButton("Send", bottomWidget);
    composerLayout->addWidget(messageComposer);
    composerLayout->addWidget(sendButton);

    connect(sendButton, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
}

void ThreadView::setMailbox(Mailbox *box)
{
    mailbox = box;
}

void ThreadView::setMessageThread(MessageThread *thread)
{
    messageDisplay->setModel(&thread->getMessages());
    messageThread = thread;

    if (messageThread->getChannelInfos().size() == 0)
        return;

    MessageChannelInfo *info = messageThread->getChannelInfos().at(0);

    subjectLabel->setText("Subject: " + info->getSubject());

    QVector<MessageChannelInfo::Participant> participantsList = info->getParticipants();
    QString participantsString = "Participants: ";
    bool first = true;
    foreach (const MessageChannelInfo::Participant &participant, participantsList) {
        if (!first)
            participantsString += ", ";
        else
            first = false;
        participantsString += participant.address;
    }
    participantsLabel->setText(participantsString);
}

void ThreadView::onSendButtonClicked()
{
    if (messageThread->getChannelInfos().size() == 0)
        return;

    MessageChannelInfo *info = messageThread->getChannelInfos().at(0);

    Message *message = new Message(info);
    QString body = messageComposer->toPlainText();
    message->setBody(body.toLatin1());

    MultiMailMessenger *messenger = new MultiMailMessenger(mailbox, profile);
    messenger->postMessage(message);

    // todo progress bar
    messageComposer->clear();
}
