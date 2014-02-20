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
    messageDisplay = new QListView(this);
    QWidget *composerWidget = new QWidget(this);
    setStretchFactor(0, 5);
    setStretchFactor(1, 2);

    QVBoxLayout* composerLayout = new QVBoxLayout();
    composerWidget->setLayout(composerLayout);

    receiver = new QLineEdit(composerWidget);
receiver->setText("lec@localhost");
    messageComposer = new QTextEdit(composerWidget);
    sendButton = new QPushButton("Send", composerWidget);
    composerLayout->addWidget(receiver);
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
}

void ThreadView::onSendButtonClicked()
{
    QString address = receiver->text();
    if (address == "")
        return;

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
