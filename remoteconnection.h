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

    QIODevice *getDevice();

    QByteArray readAll();
    virtual void abort() = 0;

signals:
    void finished(WP::err error);

protected:
    QIODevice *device;
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

    bool connected;
    bool connecting;
};


class HTTPConnectionReply : public RemoteConnectionReply {
Q_OBJECT
public:
    HTTPConnectionReply(QIODevice *getDevice, QNetworkReply *reply, QObject *parent = NULL);

    virtual void abort();

private slots:
    void finishedSlot();
    void errorSlot(QNetworkReply::NetworkError code);

private:
    QNetworkReply *networkReply;
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
    QUrl url;
    QMap<QNetworkReply*, RemoteConnectionReply*> networkReplyMap;
};


class PHPEncryptionFilter;

class PHPEncryptedDevice : public QBuffer {
Q_OBJECT
public:
    PHPEncryptedDevice(PHPEncryptionFilter *encryption, QNetworkReply *source);

protected:
    qint64 readData(char *data, qint64 maxSize);

private:
    PHPEncryptionFilter *encryption;
    QNetworkReply *source;
    bool hasBeenDecrypted;
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
    CryptoInterface *crypto;
    QNetworkReply *networkReply;

    QString secretNumber;
    QByteArray initVector;
    PHPEncryptionFilter *encryption;
};

class RemoteConnectionInfo {
public:
    enum RemoteConnectionType {
        kPlain,
        kSecure,
    };
    RemoteConnectionInfo();
    RemoteConnectionInfo& operator=(RemoteConnectionInfo info);

    QUrl getUrl() const;
    void setUrl(const QUrl &value);

    RemoteConnectionType getType() const;
    void setType(const RemoteConnectionType &value);

    friend bool operator== (RemoteConnectionInfo &info1, RemoteConnectionInfo &info2);
    friend bool operator!= (RemoteConnectionInfo &info1, RemoteConnectionInfo &info2);

private:
    QUrl url;
    RemoteConnectionType type;
};


#endif // SERVERCONNECTION_H
