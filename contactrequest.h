#ifndef CONTACTREQUEST_H
#define CONTACTREQUEST_H

#include <QObject>

#include "remoteconnection.h"


class Contact;
class UserIdentity;

/*! Request a mutal contact. This means own contact information is sent out to a remote server and
 * in exchange the remote server sends information back. This informations contains at least all
 * neccesary informations to do basic message exchange.
 */
class ContactRequest : public QObject
{
    Q_OBJECT
public:
    ContactRequest(RemoteConnection *connection, const QString &remoteServerUser,
                   UserIdentity *identity, QObject *parent = NULL);

    WP::err postRequest();

signals:
    void contactRequestFinished(WP::err code);

protected slots:
    void onConnectionRelpy(WP::err code);
    void onRequestReply(WP::err code);

private:
    void makeRequest(QByteArray &data, Contact *myself);

private:
    RemoteConnection *fConnection;
    QString fServerUser;
    UserIdentity *fUserIdentity;
    RemoteConnectionReply *fServerReply;

};

#endif // CONTACTREQUEST_H
