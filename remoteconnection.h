#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H

#include <QBuffer>
#include <QByteArray>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <cryptointerface.h>


class RemoteConnectionReply : public QObject
{
Q_OBJECT
public:
    RemoteConnectionReply(QIODevice *device, QObject *parent = NULL);
    virtual ~RemoteConnectionReply() {}

    QIODevice *device();

    QByteArray readAll();
    virtual void abort() = 0;

signals:
    void finished(WP::err error);

protected:
    QIODevice *fDevice;
};


class RemoteConnection : public QObject
{
Q_OBJECT
public:
    RemoteConnection(QObject *parent = NULL);
    virtual ~RemoteConnection();

    virtual WP::err connectToServer() = 0;
    virtual WP::err disconnectFromServer() = 0;
    virtual RemoteConnectionReply *send(const QByteArray& data) = 0;

    bool isConnected();
    bool isConnecting();

signals:
    void connectionAttemptFinished(WP::err);

protected:
    void setConnectionStarted();
    void setConnected();
    void setDisconnected();

    bool fConnected;
    bool fConnecting;
};


class HTTPConnectionReply : public RemoteConnectionReply {
Q_OBJECT
public:
    HTTPConnectionReply(QIODevice *device, QNetworkReply *reply, QObject *parent = NULL);

    virtual void abort();

private slots:
    void finishedSlot();
    void errorSlot(QNetworkReply::NetworkError code);

private:
    QNetworkReply *fReply;
};


class HTTPConnection : public RemoteConnection
{
Q_OBJECT
public:
    HTTPConnection(const QUrl &url, QObject *parent = NULL);
    virtual ~HTTPConnection();

    QUrl getUrl();

    static QNetworkAccessManager *getNetworkAccessManager();

    WP::err connectToServer();
    WP::err disconnectFromServer();
    virtual RemoteConnectionReply *send(const QByteArray& data);

protected slots:
    void replyFinished(QNetworkReply *reply);

protected:
    virtual RemoteConnectionReply* createRemoteConnectionReply(QNetworkReply *reply);

protected:
    QUrl fUrl;
    QMap<QNetworkReply*, RemoteConnectionReply*> fReplyMap;
};


class PHPEncryptionFilter;

class PHPEncryptedDevice : public QBuffer {
Q_OBJECT
public:
    PHPEncryptedDevice(PHPEncryptionFilter *encryption, QNetworkReply *source);

protected:
    qint64 readData(char *data, qint64 maxSize);

private:
    PHPEncryptionFilter *fEncryption;
    QNetworkReply *fSource;
    bool fHasBeenDecrypted;
};


class EncryptedPHPConnectionReply : public HTTPConnectionReply {
public:
    EncryptedPHPConnectionReply(PHPEncryptionFilter *encryption, QNetworkReply *reply,
                                QObject *parent = NULL);
    virtual ~EncryptedPHPConnectionReply();
};

class EncryptedPHPConnection : public HTTPConnection
{
Q_OBJECT
public:
    EncryptedPHPConnection(QUrl url, QObject *parent = NULL);

    WP::err connectToServer();
    WP::err disconnectFromServer();

    virtual RemoteConnectionReply *send(const QByteArray& data);

private slots:
    void handleConnectionAttemptReply();
    void networkRequestError(QNetworkReply::NetworkError code);

protected:
    virtual RemoteConnectionReply* createRemoteConnectionReply(QNetworkReply *reply);

private:
    CryptoInterface *fCrypto;
    QNetworkReply *fNetworkReply;

    QString fSecretNumber;
    QByteArray fInitVector;
    PHPEncryptionFilter *fEncryption;
};



template<class Type>
class ConnectionBucket
{
public:
    Type *connectionFor(const QUrl &url) {
        Type *connection = NULL;
        QString hash = url.toString();
        typename QMap<QString, Type*>::iterator it = fConnections.find(hash);
        if (it != fConnections.end()) {
            connection = it.value();
        } else {
            connection = new Type(QUrl(url));
            if (connection == NULL)
                return NULL;
            fConnections[hash] = connection;
        }
        return connection;
    }

private:
    QMap<QString, Type*> fConnections;
};


class ConnectionManager {
public:
   static RemoteConnection *defaultConnectionFor(const QUrl &url);
   static HTTPConnection *connectionHTTPFor(const QUrl &url);
   static EncryptedPHPConnection *connectionPHPFor(const QUrl &url);

private:
    static ConnectionBucket<HTTPConnection> sHTTPConnectionBucket;
    static ConnectionBucket<EncryptedPHPConnection> sPHPConnectionBucket;
};


#endif // SERVERCONNECTION_H
