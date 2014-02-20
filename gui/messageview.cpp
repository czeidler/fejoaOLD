#include "messageview.h"

#include <QVBoxLayout>

#include "newmessageview.h"
#include "profile.h"
#include "threadview.h"


MessageView::MessageView(Profile *_profile, QWidget *parent) :
    QSplitter(Qt::Horizontal, parent),
    profile(_profile),
    mailbox(NULL)
{
    QWidget *threadsWidget = new QWidget(this);
    threadsWidget->setLayout(new QVBoxLayout());

    threadListView = new QListView();
    newThreadButton = new QPushButton("New Conversation");
    threadsWidget->layout()->addWidget(threadListView);
    threadsWidget->layout()->addWidget(newThreadButton);

    stackedWidget = new QStackedWidget(this);

    threadView = new ThreadView(profile);
    newMessageView = new NewMessageView(profile);

    stackedWidget->addWidget(threadView);
    stackedWidget->addWidget(newMessageView);
    stackedWidget->setCurrentWidget(newMessageView);

    setStretchFactor(0, 2);
    setStretchFactor(1, 5);

    connect(threadListView, SIGNAL(activated(QModelIndex)), this, SLOT(onThreadSelected(QModelIndex)));
    connect(newThreadButton, SIGNAL(clicked()), this, SLOT(onSendButtonClicked()));
}

void MessageView::setMailbox(Mailbox *_mailbox)
{
    mailbox = _mailbox;

    threadView->setMailbox(mailbox);
    newMessageView->setMailbox(mailbox);
    threadListView->setModel(&mailbox->getThreads());
}

void MessageView::onThreadSelected(QModelIndex index)
{
    stackedWidget->setCurrentWidget(threadView);

    MessageThread *messageThread = mailbox->getThreads().channelAt(index.row());
    threadView->setMessageThread(messageThread);
}

void MessageView::onSendButtonClicked() {
    stackedWidget->setCurrentWidget(newMessageView);
    threadListView->clearSelection();
    threadListView->clearFocus();
}

