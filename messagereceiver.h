#ifndef MESSAGERECEIVER_H
#define MESSAGERECEIVER_H

#include <QObject>

#include <gitinterface.h>



class MessageReceiver : public QObject
{
    Q_OBJECT
public:
    explicit MessageReceiver(GitInterface* gitInterface, QObject *parent = 0);

    Q_INVOKABLE void messageDataReceived(const QByteArray &data);
    Q_INVOKABLE QString getMessagesRequest();

    void    commitPackReceived(const QByteArray& data, const QString &branch, const QString &first, const QString &last);
signals:
    
public slots:

private:
    int _ReadTill(QByteArray &in, QString& out, int start, char stopChar);
    QString _CreateXMLMessageRequest(const QString& branch, const QString& fromCommit, const QString& toCommit);


    GitInterface*   fGitInterface;
};

#endif // MESSAGERECEIVER_H
