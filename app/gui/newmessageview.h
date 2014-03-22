#ifndef NEWMESSAGEVIEW_H
#define NEWMESSAGEVIEW_H

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>

class Mailbox;
class Profile;

class NewMessageView : public QWidget
{
    Q_OBJECT
public:
    explicit NewMessageView(Profile *profile, QWidget *parent = 0);

    void setMailbox(Mailbox *mailbox);

signals:

public slots:

private slots:
    void onSendButtonClicked();
private:
    QLineEdit *receiverEdit;
    QLineEdit *subjectEdit;
    QTextEdit *messageComposer;
    QPushButton *sendButton;

    Profile *profile;
    Mailbox *mailbox;
};

#endif // NEWMESSAGEVIEW_H
