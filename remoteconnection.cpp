#include "remoteconnection.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHttpPart>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>

#include "profile.h"
#include "mainapplication.h"


class PHPEncryptionFilter {
public:
    PHPEncryptionFilter(CryptoInterface *crypto, const SecureArray &cipherKey,
                        const QByteArray &iv);
    //! called before send data
    virtual void sendFilter(const QByteArray &in, QByteArray &out);
    //! called when receive data
    virtual void receiveFilter(const QByteArray &in, QByteArray &out);
private:
    CryptoInterface *fCrypto;
    SecureArray fCipherKey;
    QByteArray fIV;
};

RemoteConnectionReply::RemoteConnectionReply(QIODevice *device, QObject *parent) :
    QObject(parent),
    fDevice(device)
{
}

QIODevice *RemoteConnectionReply::device()
{
    return fDevice;
}

QByteArray RemoteConnectionReply::readAll()
{
    QByteArray data = fDevice->readAll();
    qDebug() << data;
    return data;
    //return fDevice->readAll();
}

RemoteConnection::RemoteConnection(QObject *parent) :
    QObject(parent),
    fConnected(false),
    fConnecting(false)
{
}

RemoteConnection::~RemoteConnection()
{
}

bool RemoteConnection::isConnected()
{
    return fConnected;
}

bool RemoteConnection::isConnecting()
{
    return fConnecting;
}

void RemoteConnection::setConnectionStarted()
{
    fConnecting = true;
    fConnected = false;
}

void RemoteConnection::setConnected()
{
    fConnected = true;
    fConnecting = false;
}

void RemoteConnection::setDisconnected()
{
    fConnecting = false;
    fConnected = false;
}


HTTPConnectionReply::HTTPConnectionReply(QIODevice *device, QNetworkReply *reply, QObject *parent) :
    RemoteConnectionReply(device, parent),
    fReply(reply)
{
    connect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(errorSlot(QNetworkReply::NetworkError)));
}

void HTTPConnectionReply::abort()
{
    if (!fReply->isFinished())
        fReply->abort();
}

void HTTPConnectionReply::finishedSlot()
{
    emit finished(WP::kOk);
}

void HTTPConnectionReply::errorSlot(QNetworkReply::NetworkError code)
{
    emit finished(WP::kError);
}

HTTPConnection::HTTPConnection(const QUrl &url, QObject *parent) :
    RemoteConnection(parent),
    fUrl(url)
{
}

HTTPConnection::~HTTPConnection()
{
}

QUrl HTTPConnection::getUrl()
{
    return fUrl;
}


QNetworkAccessManager* HTTPConnection::getNetworkAccessManager()
{
    return ((MainApplication*)qApp)->getNetworkAccessManager();
}

WP::err HTTPConnection::connectToServer()
{
    setConnected();
    emit connectionAttemptFinished(WP::kOk);
    return WP::kOk;
}

WP::err HTTPConnection::disconnectFromServer()
{
    setDisconnected();
    return WP::kOk;
}

RemoteConnectionReply *HTTPConnection::send(const QByteArray &data)
{
     QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

     QHttpPart previewPathPart;
     previewPathPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               QVariant("form-data; name=\"transfer_data\""));
     previewPathPart.setBody("transfer_data.txt");

     QHttpPart previewFilePart;
     previewFilePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
     previewFilePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               QVariant("form-data; name=\"transfer_data\"; filename=\"transfer_data.txt\""));

     previewFilePart.setBody(data);

     multiPart->append(previewPathPart);
     multiPart->append(previewFilePart);

     QNetworkRequest request(fUrl);
/*
    QNetworkRequest request;
    request.setUrl(fUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QNetworkAccessManager *manager = getNetworkAccessManager();
    QByteArray outgoing = data;
    outgoing.prepend("request=");
    */
    QNetworkAccessManager *manager = getNetworkAccessManager();

    QNetworkReply *reply = manager->post(request, multiPart);
    if (reply == NULL)
        return NULL;

    manager->disconnect(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    //connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));

    RemoteConnectionReply *remoteConnectionReply = createRemoteConnectionReply(reply);
    fReplyMap.insert(reply, remoteConnectionReply);

    return remoteConnectionReply;
}

void HTTPConnection::replyFinished(QNetworkReply *reply)
{
    QMap<QNetworkReply*, RemoteConnectionReply*>::iterator it = fReplyMap.find(reply);
    if (it == fReplyMap.end())
        return;

    RemoteConnectionReply* receiver = it.value();
    receiver->deleteLater();
    fReplyMap.remove(reply);
}

RemoteConnectionReply *HTTPConnection::createRemoteConnectionReply(QNetworkReply *reply)
{
    return new HTTPConnectionReply(reply, reply, this);
}

EncryptedPHPConnection::EncryptedPHPConnection(QUrl url, QObject *parent) :
    HTTPConnection(url, parent),
    fEncryption(NULL)
{
    fCrypto = CryptoInterfaceSingleton::getCryptoInterface();
}

WP::err EncryptedPHPConnection::connectToServer()
{
    if (isConnected())
        return WP::kIsConnected;
    if (isConnecting())
        return WP::kOk;

    fInitVector = fCrypto->generateInitalizationVector(512);
    QString prime, base, pub;
    fCrypto->generateDHParam(prime, base, fSecretNumber, pub);

    QNetworkAccessManager *manager = HTTPConnection::getNetworkAccessManager();

    QNetworkRequest request;
    request.setUrl(fUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QByteArray content = "";
    content += "request=neqotiate_dh_key&";
    content += "dh_prime=" + prime + "&";
    content += "dh_base=" + base + "&";
    content += "dh_public_key=" + pub + "&";
    content += "encrypt_iv=" + fInitVector.toBase64();

    fNetworkReply = manager->post(request, content);
    connect(fNetworkReply, SIGNAL(finished()), this, SLOT(handleConnectionAttemptReply()));
    connect(fNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this,
            SLOT(networkRequestError(QNetworkReply::NetworkError)));

    setConnectionStarted();
    return WP::kOk;
}

WP::err EncryptedPHPConnection::disconnectFromServer()
{
    // TODO notify the server
    setDisconnected();
    return WP::kOk;
}

RemoteConnectionReply *EncryptedPHPConnection::send(const QByteArray &data)
{
    if (fEncryption == NULL)
        return NULL;

    QByteArray outgoing;
    fEncryption->sendFilter(data, outgoing);
    return HTTPConnection::send(outgoing);
}

void EncryptedPHPConnection::handleConnectionAttemptReply()
{
    QByteArray data = fNetworkReply->readAll();

    QString prime;
    QString base;
    QString publicKey;

    QXmlStreamReader readerXML(data);
    while (!readerXML.atEnd()) {
        switch (readerXML.readNext()) {
        case QXmlStreamReader::EndElement:
            break;
        case QXmlStreamReader::StartElement:
            if (readerXML.name().compare("neqotiated_dh_key", Qt::CaseInsensitive) == 0) {
                QXmlStreamAttributes attributes = readerXML.attributes();
                if (attributes.hasAttribute("dh_prime"))
                    prime = attributes.value("dh_prime").toString();;
                if (attributes.hasAttribute("dh_base"))
                    base = attributes.value("dh_base").toString();;
                if (attributes.hasAttribute("dh_public_key"))
                    publicKey = attributes.value("dh_public_key").toString();;

            }
            break;
        default:
            break;
        }
    }
    if (prime == "" || base == "" || publicKey == "")
        return;
    SecureArray key = fCrypto->sharedDHKey(prime, publicKey, fSecretNumber);
    // make it at least 128 byte
    for (int i = key.count(); i < 128; i++)
        key.append('\0');

    fEncryption = new PHPEncryptionFilter(fCrypto, key, fInitVector);

    fNetworkReply->deleteLater();
    setConnected();
    emit connectionAttemptFinished(WP::kOk);
}

void EncryptedPHPConnection::networkRequestError(QNetworkReply::NetworkError code)
{
    setDisconnected();
    fNetworkReply->deleteLater();
    emit connectionAttemptFinished(WP::kError);
}

RemoteConnectionReply *EncryptedPHPConnection::createRemoteConnectionReply(QNetworkReply *reply)
{
    return new EncryptedPHPConnectionReply(fEncryption, reply, this);
}


PHPEncryptionFilter::PHPEncryptionFilter(CryptoInterface *crypto,
                                         const SecureArray &cipherKey,
                                         const QByteArray &iv) :
    fCrypto(crypto),
    fCipherKey(cipherKey),
    fIV(iv)
{
}

void PHPEncryptionFilter::sendFilter(const QByteArray &in, QByteArray &out)
{
    fCrypto->encryptSymmetric(in, out, fCipherKey, fIV, "aes128");
    out = out.toBase64();
}

void PHPEncryptionFilter::receiveFilter(const QByteArray &in, QByteArray &out)
{
    fCrypto->decryptSymmetric(in, out, fCipherKey, fIV, "aes128");
}

PHPEncryptedDevice::PHPEncryptedDevice(PHPEncryptionFilter *encryption, QNetworkReply *source) :
    fEncryption(encryption),
    fSource(source),
    fHasBeenDecrypted(false)
{
}

qint64 PHPEncryptedDevice::readData(char *data, qint64 maxSize)
{
    if (!fSource->isFinished())
        return 0;
    if (!fHasBeenDecrypted) {
        const QByteArray &encryptedData = fSource->readAll();
        fEncryption->receiveFilter(encryptedData, buffer());
        fHasBeenDecrypted = true;
        emit readyRead();
    }
    return QBuffer::readData(data, maxSize);
}


EncryptedPHPConnectionReply::EncryptedPHPConnectionReply(PHPEncryptionFilter *encryption, QNetworkReply *reply,
                                                         QObject *parent) :
    HTTPConnectionReply(new PHPEncryptedDevice(encryption, reply), reply, parent)
{
    fDevice->open(QIODevice::ReadOnly);
}

EncryptedPHPConnectionReply::~EncryptedPHPConnectionReply()
{
    delete fDevice;
}


ConnectionBucket<HTTPConnection> ConnectionManager::sHTTPConnectionBucket;
ConnectionBucket<EncryptedPHPConnection> ConnectionManager::sPHPConnectionBucket;

RemoteConnection *ConnectionManager::defaultConnectionFor(const QUrl &url)
{
    return connectionHTTPFor(url);
}

HTTPConnection *ConnectionManager::connectionHTTPFor(const QUrl &url)
{
    return sHTTPConnectionBucket.connectionFor(url);
}

EncryptedPHPConnection *ConnectionManager::connectionPHPFor(const QUrl &url)
{
    return sPHPConnectionBucket.connectionFor(url);
}
