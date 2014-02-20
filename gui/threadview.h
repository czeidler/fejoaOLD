#ifndef THREADVIEW_H
#define THREADVIEW_H

#include <QPushButton>
#include <QLineEdit>
#include <QListView>
#include <QSplitter>
#include <QTextEdit>

class Mailbox;
class MessageListModel;
class MessageThread;
class Profile;

class ThreadView: public QSplitter
{
    Q_OBJECT
public:
    explicit ThreadView(Profile *profile, QWidget *parent = 0);

    void setMailbox(Mailbox *mailbox);
    void setMessageThread(MessageThread *thread);

signals:

public slots:

private slots:
    void onSendButtonClicked();

private:
    QListView *messageDisplay;
    QLineEdit *receiver;
    QTextEdit *messageComposer;
    QPushButton *sendButton;

    Mailbox *mailbox;
    Profile *profile;
    MessageThread* messageThread;
};

#endif // THREADVIEW_H
