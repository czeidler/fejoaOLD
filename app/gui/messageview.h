#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <QPushButton>
#include <QListView>
#include <QSplitter>
#include <QStackedWidget>

class Mailbox;
class NewMessageView;
class Profile;
class ThreadView;

class MessageView : public QSplitter
{
    Q_OBJECT
public:
    explicit MessageView(Profile *profile, QWidget *parent = 0);
    
    void setMailbox(Mailbox *mailbox);

private slots:
    void onThreadSelected(QModelIndex index);
    void onSendButtonClicked();

private:
    Profile *profile;
    Mailbox *mailbox;

    QListView *threadListView;
    QPushButton *newThreadButton;

    QStackedWidget *stackedWidget;
    ThreadView *threadView;
    NewMessageView *newMessageView;
};

#endif // MESSAGEVIEW_H
