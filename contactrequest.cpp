#include "contactrequest.h"

#include "contact.h"
#include "protocolparser.h"
#include "useridentity.h"


const char *kContactRequestStanza = "contact_request";


class ContactRequestHandler : public InStanzaHandler {
public:
    ContactRequestHandler() :
        InStanzaHandler(kContactRequestStanza)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        if (!attributes.hasAttribute("status"))
            return false;
        status = attributes.value("status").toString();
        if (status != "ok")
            return true;

        if (!attributes.hasAttribute("uid"))
            return false;
        if (!attributes.hasAttribute("keyId"))
            return false;
        if (!attributes.hasAttribute("address"))
            return false;

        uid = attributes.value("uid").toString();
        keyId = attributes.value("keyId").toString();
        address = attributes.value("address").toString();

        if (uid == "")
            return false;

        return true;
    }

public:
    QString status;
    QString uid;
    QString address;
    QString keyId;
};

class CertificateHandler : public InStanzaHandler {
public:
    CertificateHandler() :
        InStanzaHandler("certificate", true)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        return true;
    }

    bool handleText(const QStringRef &text) {
        certificate = text.toString();
        return true;
    }

public:
    QString certificate;
};

class PublicKeyHandler : public InStanzaHandler {
public:
    PublicKeyHandler() :
        InStanzaHandler("publicKey", true)
    {
    }

    bool handleStanza(const QXmlStreamAttributes &attributes)
    {
        return true;
    }

    bool handleText(const QStringRef &text) {
        publicKey = text.toString();
        return true;
    }

public:
    QString publicKey;
};


ContactRequest::ContactRequest(RemoteConnection *connection, const QString &remoteServerUser,
                               UserIdentity *identity, QObject *parent) :
    QObject(parent),
    fConnection(connection),
    fServerUser(remoteServerUser),
    fUserIdentity(identity),
    fServerReply(NULL)
{
}

WP::err ContactRequest::postRequest()
{
    if (fConnection->isConnected())
        onConnectionRelpy(WP::kOk);
    else {
        connect(fConnection, SIGNAL(connectionAttemptFinished(WP::err)), this,
                SLOT(onConnectionRelpy(WP::err)));
        fConnection->connectToServer();
    }
    return WP::kOk;
}

void ContactRequest::onConnectionRelpy(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data;
    makeRequest(data, fUserIdentity->getMyself());

    fServerReply = fConnection->send(data);
    connect(fServerReply, SIGNAL(finished(WP::err)), this,
            SLOT(onRequestReply(WP::err)));
}

void ContactRequest::onRequestReply(WP::err code)
{
    if (code != WP::kOk)
        return;

    QByteArray data = fServerReply->readAll();
    fServerReply = NULL;

    IqInStanzaHandler iqHandler(kResult);
    ContactRequestHandler *requestHandler = new ContactRequestHandler();
    CertificateHandler *certificateHandler = new CertificateHandler();
    PublicKeyHandler *publicKeyHandler = new PublicKeyHandler();
    iqHandler.addChildHandler(requestHandler);
    requestHandler->addChildHandler(certificateHandler);
    requestHandler->addChildHandler(publicKeyHandler);

    ProtocolInStream inStream(data);
    inStream.addHandler(&iqHandler);

    inStream.parse();

    if (!requestHandler->hasBeenHandled()) {
        emit contactRequestFinished(WP::kBadValue);
        return;
    }

    if (requestHandler->status != "ok") {
        emit contactRequestFinished(WP::kContactRefused);
        return;
    }

    if (!certificateHandler->hasBeenHandled() || !publicKeyHandler->hasBeenHandled()) {
        emit contactRequestFinished(WP::kBadValue);
        return;
    }

    /*CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    if (crypto->verifySignatur(certificateHandler->certificate.toLatin1(),
                               requestHandler->certificateSignature.toLatin1(),
                               publicKeyHandler->publicKey)) {
        emit contactRequestFinished(WP::kBadValue);
        return;
    }*/
    Contact *contact = new Contact(requestHandler->uid, requestHandler->keyId,
                                   certificateHandler->certificate, publicKeyHandler->publicKey);
    contact->setAddress(requestHandler->address);

    WP::err error = fUserIdentity->addContact(contact);
    if (error != WP::kOk) {
        delete contact;
        emit contactRequestFinished(WP::kError);
        return;
    }
    // commit changes
    fUserIdentity->getDatabaseBranch()->commit();

    emit contactRequestFinished(WP::kOk);
    return;
}

void ContactRequest::makeRequest(QByteArray &data, Contact *myself)
{
    ProtocolOutStream outStream(&data);
    IqOutStanza *iqStanza = new IqOutStanza(kGet);
    outStream.pushStanza(iqStanza);

    QString keyId = myself->getKeys()->getMainKeyId();
    QString certificate, publicKey, privateKey;
    fUserIdentity->getKeyStore()->readAsymmetricKey(keyId, certificate, publicKey, privateKey);

    OutStanza *requestStanza =  new OutStanza(kContactRequestStanza);
    requestStanza->addAttribute("serverUser", fServerUser);
    requestStanza->addAttribute("uid", myself->getUid());
    requestStanza->addAttribute("keyId", keyId);
    requestStanza->addAttribute("address", myself->getAddress());
    outStream.pushChildStanza(requestStanza);

    OutStanza *certificateStanza = new OutStanza("certificate");
    certificateStanza->setText(certificate);
    outStream.pushChildStanza(certificateStanza);
    outStream.cdDotDot();

    OutStanza *publicKeyStanza = new OutStanza("publicKey");
    publicKeyStanza->setText(publicKey);
    outStream.pushChildStanza(publicKeyStanza);
    outStream.cdDotDot();

    outStream.flush();
}
