#include "mail.h"

#include <QBuffer>
#include <QString>


const QByteArray DataParcel::getSignature() const
{
    return signature;
}

const QString DataParcel::getSignatureKey() const
{
    return signatureKey;
}

const Contact *DataParcel::getSender() const
{
    return sender;
}

QString DataParcel::getUid() const
{
    return uid;
}

WP::err DataParcel::toRawData(Contact *_sender, const QString &_signatureKey, QIODevice &rawData)
{
    sender = _sender;
    signatureKey = _signatureKey;

    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();


    QByteArray containerData;
    QDataStream containerDataStream(&containerData, QIODevice::WriteOnly);

    containerDataStream.device()->write(sender->getUid().toLatin1());
    containerDataStream << (qint8)0;
    containerDataStream.device()->write(signatureKey.toLatin1());
    containerDataStream << (qint8)0;

    QByteArray mainData;
    QDataStream mainDataStream(&mainData, QIODevice::WriteOnly);

    WP::err error = writeMainData(mainDataStream);
    if (error != WP::kOk)
        return error;
    uid = crypto->toHex(crypto->sha1Hash(mainData));

    containerDataStream << mainData.length();
    if (containerDataStream.device()->write(mainData) != mainData.length())
        return WP::kError;

    QByteArray signatureHash = crypto->toHex(crypto->sha2Hash(containerData)).toLatin1();
    error = sender->sign(signatureKey, signatureHash, signature);
    if (error != WP::kOk)
        return error;

    // write signature header
    QDataStream rawDataStream(&rawData);
    rawDataStream << signature.length();
    if (rawData.write(signature) != signature.length())
        return WP::kError;
    if (rawData.write(containerData) != containerData.length())
        return WP::kError;

    return WP::kOk;
}

QString DataParcel::readString(QIODevice &data) const
{
    QString string;
    char c;
    while (data.getChar(&c)) {
        if (c == '\0')
            break;
        string += c;
    }
    return string;
}

WP::err DataParcel::fromRawData(ContactFinder *contactFinder, QByteArray &rawData)
{
    QDataStream stream(&rawData, QIODevice::ReadOnly);
    quint32 signatureLength;
    stream >> signatureLength;
    if (signatureLength <= 0 || signatureLength >= (quint32)rawData.size())
        return WP::kBadValue;

    char *buffer = new char[signatureLength];
    stream.device()->read(buffer, signatureLength);
    signature.clear();
    signature.append(buffer, signatureLength);
    delete[] buffer;

    int position = stream.device()->pos();
    QByteArray signedData;
    signedData.setRawData(rawData.data() + position, rawData.length() - position);
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    QByteArray signatureHash = crypto->toHex(crypto->sha2Hash(signedData)).toLatin1();

    QString senderUid = readString(*stream.device());
    signatureKey = readString(*stream.device());

    sender = contactFinder->find(senderUid);
    if (sender == NULL)
        return WP::kContactNotFound;

    int mainDataLength;
    stream >> mainDataLength;

    // TODO do proper QIODevice reading
    QByteArray mainData = stream.device()->read(mainDataLength);
    if (mainData.length() != mainDataLength)
        return WP::kError;
    uid = crypto->toHex(crypto->sha1Hash(mainData));

    QBuffer mainDataBuffer(&mainData);
    mainDataBuffer.open(QBuffer::ReadOnly);
    WP::err error = readMainData(mainDataBuffer);
    if (error != WP::kOk)
        return error;

    // validate data
    if (!sender->verify(signatureKey, signatureHash, signature))
        return WP::kBadValue;

    return WP::kOk;
}

void ParcelCrypto::initNew()
{
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    iv = crypto->generateInitalizationVector(kSymmetricKeySize);
    symmetricKey = crypto->generateSymmetricKey(kSymmetricKeySize);
}

WP::err ParcelCrypto::initFromPublic(Contact *receiver, const QString &keyId, const QByteArray &iv, const QByteArray &encryptedSymmetricKey)
{
    this->iv = iv;

    QString certificate;
    QString publicKey;
    QString privateKey;
    WP::err error = receiver->getKeys()->getKeySet(keyId, certificate, publicKey, privateKey);
    if (error != WP::kOk)
        return error;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    error = crypto->decryptAsymmetric(encryptedSymmetricKey, symmetricKey, privateKey, "", certificate);
    if (error != WP::kOk)
        return error;
    return error;
}

void ParcelCrypto::initFromPrivate(const QByteArray &_iv, const QByteArray &_symmetricKey)
{
    iv = _iv;
    symmetricKey = _symmetricKey;
}

WP::err ParcelCrypto::cloakData(const QByteArray &data, QByteArray &cloakedData)
{
    if (symmetricKey.count() == 0)
        return WP::kNotInit;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    WP::err error = crypto->encryptSymmetric(data, cloakedData, symmetricKey, iv);
    return error;
}

WP::err ParcelCrypto::uncloakData(const QByteArray &cloakedData, QByteArray &data)
{
    if (symmetricKey.count() == 0)
        return WP::kNotInit;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    WP::err error = crypto->decryptSymmetric(cloakedData, data, symmetricKey, iv);
    return error;
}

const QByteArray &ParcelCrypto::getIV() const
{
    return iv;
}

const QByteArray &ParcelCrypto::getSymmetricKey() const
{
    return symmetricKey;
}

WP::err ParcelCrypto::getEncryptedSymmetricKey(Contact *receiver, const QString &keyId, QByteArray &encryptedSymmetricKey)
{
    QString certificate;
    QString publicKey;
    WP::err error = receiver->getKeys()->getKeySet(keyId, certificate, publicKey);
    if (error != WP::kOk)
        return error;
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    error = crypto->encyrptAsymmetric(symmetricKey, encryptedSymmetricKey, certificate);
    if (error != WP::kOk)
        return error;
    return error;
}


SecureChannel::SecureChannel(SecureChannelRef channel, Contact *_receiver, const QString &asymKeyId) :
    AbstractSecureDataParcel(channel->getType()),
    receiver(_receiver),
    asymmetricKeyId(asymKeyId)
{
    parcelCrypto = channel->parcelCrypto;
}

SecureChannel::SecureChannel(qint8 type, Contact *_receiver) :
    AbstractSecureDataParcel(type),
    receiver(_receiver)
{

}

SecureChannel::SecureChannel(qint8 type, Contact *_receiver, const QString &asymKeyId) :
    AbstractSecureDataParcel(type),
    receiver(_receiver),
    asymmetricKeyId(asymKeyId)
{
    parcelCrypto.initNew();
}

SecureChannel::~SecureChannel()
{
}

WP::err SecureChannel::writeDataSecure(QDataStream &stream, const QByteArray &data)
{
    QByteArray encryptedData;
    WP::err error = parcelCrypto.cloakData(data, encryptedData);
    if (error != WP::kOk)
        return error;

    stream << encryptedData.length();
    if (stream.device()->write(encryptedData) != encryptedData.length())
        return WP::kError;
    return WP::kOk;
}

WP::err SecureChannel::readDataSecure(QDataStream &stream, QByteArray &data)
{
    int length;
    stream >> length;
    QByteArray encryptedData = stream.device()->read(length);

    return parcelCrypto.uncloakData(encryptedData, data);
}

WP::err SecureChannel::writeMainData(QDataStream &stream)
{
    QByteArray encryptedSymmetricKey;
    WP::err error = parcelCrypto.getEncryptedSymmetricKey(receiver, asymmetricKeyId, encryptedSymmetricKey);
    if (error != WP::kOk)
        return error;

    stream.device()->write(asymmetricKeyId.toLatin1());
    stream << (qint8)0;
    stream << encryptedSymmetricKey.length();
    stream.device()->write(encryptedSymmetricKey);
    stream << parcelCrypto.getIV().length();
    stream.device()->write(parcelCrypto.getIV());
    return WP::kOk;
}

WP::err SecureChannel::readMainData(QBuffer &mainData)
{
    QDataStream inStream(&mainData);

    char *buffer;

    asymmetricKeyId = readString(mainData);

    int length;
    inStream >> length;
    buffer = new char[length];
    inStream.device()->read(buffer, length);
    QByteArray encryptedSymmetricKey;
    encryptedSymmetricKey.append(buffer, length);
    delete[] buffer;

    inStream >> length;
    buffer = new char[length];
    inStream.device()->read(buffer, length);
    QByteArray iv;
    iv.append(buffer, length);
    delete[] buffer;

    return parcelCrypto.initFromPublic(receiver, asymmetricKeyId, iv, encryptedSymmetricKey);
}

QString SecureChannel::getUid() const
{
    QByteArray data;
    data += parcelCrypto.getIV();
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();
    return crypto->toHex(crypto->sha1Hash(data));
}


SecureChannelParcel::SecureChannelParcel(qint8 type, SecureChannelRef _channel) :
    AbstractSecureDataParcel(type),
    channel(_channel)
{
    timestamp = time(NULL);
}

void SecureChannelParcel::setChannel(SecureChannelRef channel)
{
    this->channel = channel;
}

SecureChannelRef SecureChannelParcel::getChannel() const
{
    return channel;
}

time_t SecureChannelParcel::getTimestamp() const
{
    return timestamp;
}

WP::err SecureChannelParcel::writeMainData(QDataStream &stream)
{
    stream.device()->write(channel->getUid().toLatin1());
    stream << (qint8)0;

    QByteArray secureData;
    QDataStream secureStream(&secureData, QIODevice::WriteOnly);
    WP::err error = writeConfidentData(secureStream);
    if (error != WP::kOk)
        return error;

    return channel->writeDataSecure(stream, secureData);
}

WP::err SecureChannelParcel::readMainData(QBuffer &mainData)
{
    QDataStream stream(&mainData);

    QString channelId = readString(mainData);
    channel = findChannel(channelId);
    if (channel == NULL)
        return WP::kError;

    QByteArray confidentData;
    WP::err error = channel->readDataSecure(stream, confidentData);
    if (error != WP::kOk)
        return error;

    QBuffer confidentDataBuffer(&confidentData);
    confidentDataBuffer.open(QBuffer::ReadOnly);
    return readConfidentData(confidentDataBuffer);
}

WP::err SecureChannelParcel::writeConfidentData(QDataStream &stream)
{
    stream << (qint32)timestamp;

    return WP::kOk;
}

WP::err SecureChannelParcel::readConfidentData(QBuffer &mainData)
{
    QDataStream stream(&mainData);

    qint32 time;
    stream >> time;
    timestamp = time;

    return WP::kOk;
}


WP::err XMLSecureParcel::write(ProtocolOutStream *outStream, Contact *sender,
                               const QString &signatureKeyId, DataParcel *parcel,
                               const QString &stanzaName)
{
    QBuffer data;
    data.open(QBuffer::WriteOnly);
    WP::err error = parcel->toRawData(sender, signatureKeyId, data);
    if (error != WP::kOk)
        return error;

    OutStanza *parcelStanza = new OutStanza(stanzaName);
    parcelStanza->addAttribute("uid", parcel->getUid());
    parcelStanza->addAttribute("sender", parcel->getSender()->getUid());
    parcelStanza->addAttribute("signatureKey", parcel->getSignatureKey());
    parcelStanza->addAttribute("signature", parcel->getSignature().toBase64());

    parcelStanza->setText(data.buffer().toBase64());

    outStream->pushChildStanza(parcelStanza);
    outStream->cdDotDot();
    return WP::kOk;
}


MessageChannel::MessageChannel(MessageChannelRef channel, Contact *receiver, const QString &asymKeyId) :
    SecureChannel(channel, receiver, asymKeyId),
    newLocaleInfo(false)
{
    parentChannelUid = channel->parentChannelUid;
}

MessageChannel::MessageChannel(Contact *receiver) :
    SecureChannel(kMessageChannelId, receiver),
    newLocaleInfo(true)
{

}

MessageChannel::MessageChannel(Contact *receiver, const QString &asymKeyId, MessageChannelRef parent) :
    SecureChannel(kMessageChannelId, receiver, asymKeyId),
    newLocaleInfo(true)
{
    if (parent != NULL)
        parentChannelUid =parent->getUid();
}

WP::err MessageChannel::writeMainData(QDataStream &stream)
{
    WP::err error = SecureChannel::writeMainData(stream);
    if (error != WP::kOk)
        return error;

    QByteArray extra;
    QDataStream extraStream(&extra, QIODevice::WriteOnly);
    error = writeConfidentData(extraStream);
    if (error != WP::kOk)
        return error;

    return writeDataSecure(stream, extra);
}

WP::err MessageChannel::readMainData(QBuffer &mainData)
{
    WP::err error = SecureChannel::readMainData(mainData);
    if (error != WP::kOk)
        return error;

    QDataStream inStream(&mainData);
    QByteArray extra;
    error = SecureChannel::readDataSecure(inStream, extra);
    if (error != WP::kOk)
        return error;

    QBuffer extraBuffer(&extra);
    extraBuffer.open(QBuffer::ReadOnly);
    return readConfidentData(extraBuffer);
}

WP::err MessageChannel::writeConfidentData(QDataStream &stream)
{
    WP::err error = SecureChannel::writeConfidentData(stream);
    if (error != WP::kOk)
        return error;

    stream.device()->write(parentChannelUid.toLatin1());
    stream << (qint8)0;

    return WP::kOk;
}

WP::err MessageChannel::readConfidentData(QBuffer &mainData)
{
    WP::err error = SecureChannel::readConfidentData(mainData);
    if (error != WP::kOk)
        return error;

    parentChannelUid = readString(mainData);
    newLocaleInfo = false;
    return WP::kOk;
}

QString MessageChannel::getParentChannelUid() const
{
    return parentChannelUid;
}

bool MessageChannel::isNewLocale() const
{
    return newLocaleInfo;
}


MessageChannelInfo::MessageChannelInfo(MessageChannelFinder *_channelFinder) :
    SecureChannelParcel(kMessageChannelInfoId),
    channelFinder(_channelFinder),
    newLocaleInfo(true)
{

}

MessageChannelInfo::MessageChannelInfo(MessageChannelRef channel) :
    SecureChannelParcel(kMessageChannelInfoId, channel),
    channelFinder(NULL),
    newLocaleInfo(true)
{

}

void MessageChannelInfo::setSubject(const QString &subject)
{
    this->subject = subject;
}

const QString &MessageChannelInfo::getSubject() const
{
    return subject;
}

void MessageChannelInfo::addParticipant(const QString &address, const QString &uid)
{
    Participant participant;
    participant.address = address;
    participant.uid = uid;
    participants.append(participant);
}

bool MessageChannelInfo::setParticipantUid(const QString &address, const QString &uid)
{
    for (int i = 0; i < participants.size(); i++) {
        Participant &participant = participants[i];
        if (participant.address == address) {
            participant.uid = uid;
            return true;
        }
    }
    return false;
}

bool MessageChannelInfo::isNewLocale() const
{
    return newLocaleInfo;
}

QVector<MessageChannelInfo::Participant> &MessageChannelInfo::getParticipants()
{
    return participants;
}

WP::err MessageChannelInfo::writeConfidentData(QDataStream &stream)
{
    WP::err error = SecureChannelParcel::writeConfidentData(stream);
    if (error != WP::kOk)
        return error;

    if (subject != "") {
        stream << (qint8)kSubject;
        stream.device()->write(subject.toLatin1());
        stream << (qint8)0;
    }

    if (participants.count() > 0) {
        stream << (qint8)kParticipants;
        stream << (qint32)participants.count();
        foreach (const Participant &participant, participants) {
            stream.device()->write(participant.address.toLatin1());
            stream << (qint8)0;
            stream.device()->write(participant.uid.toLatin1());
            stream << (qint8)0;
        }
    }
    return WP::kOk;
}

WP::err MessageChannelInfo::readConfidentData(QBuffer &mainData)
{
    WP::err error = SecureChannelParcel::readConfidentData(mainData);
    if (error != WP::kOk)
        return error;

    QDataStream stream(&mainData);

    while (!stream.atEnd()) {
        qint8 partId;
        stream >> partId;
        if (partId == kSubject) {
            subject = readString(mainData);
        } else if (partId == kParticipants) {
            WP::err error = readParticipants(stream);
            if (error != WP::kOk)
                return error;
        } else
            return WP::kBadValue;
    }
    newLocaleInfo = false;
    return WP::kOk;
}

SecureChannelRef MessageChannelInfo::findChannel(const QString &channelUid)
{
    return channelFinder->findChannel(channelUid);
}

WP::err MessageChannelInfo::readParticipants(QDataStream &stream) {
    qint32 numberOfParticipants;
    stream >> numberOfParticipants;
    for (int i = 0; i < numberOfParticipants; i++) {
        Participant participant;
        participant.address = readString(*stream.device());
        if (participant.address == "")
            return WP::kBadValue;
        participant.uid = readString(*stream.device());
        // TODO: decide if the uid is mandatory
        //if (participant.uid == "")
        //    return WP::kBadValue;
        participants.append(participant);
    }
    return WP::kOk;
}


Message::Message(MessageChannelFinder *_channelFinder) :
    SecureChannelParcel(kMessageChannelId),
    channelInfo(NULL),
    channelFinder(_channelFinder)
{

}

Message::Message(MessageChannelInfoRef info) :
    SecureChannelParcel(kMessageChannelId, info->getChannel()),
    channelInfo(info)
{
    setChannel(info->getChannel());
}

Message::~Message()
{
}

MessageChannelInfoRef Message::getChannelInfo() const
{
    return channelInfo;
}

const QByteArray &Message::getBody() const
{
    return body;
}

void Message::setBody(const QByteArray &_body)
{
    body = _body;
}

WP::err Message::writeConfidentData(QDataStream &stream)
{
    WP::err error = SecureChannelParcel::writeConfidentData(stream);
    if (error != WP::kOk)
        return error;

    stream.device()->write(channelInfo->getUid().toLatin1());
    stream << (qint8)0;

    stream.device()->write(body);
    return WP::kOk;
}

WP::err Message::readConfidentData(QBuffer &mainData)
{
    WP::err error = SecureChannelParcel::readConfidentData(mainData);
    if (error != WP::kOk)
        return error;

    QString channelInfoUid = readString(mainData);
    channelInfo = channelFinder->findChannelInfo(getChannel()->getUid(), channelInfoUid);
    if (channelInfo == NULL)
        return WP::kBadValue;

    body = mainData.readAll();
    return WP::kOk;
}

SecureChannelRef Message::findChannel(const QString &channelUid)
{
    return channelFinder->findChannel(channelUid);
}


AbstractSecureDataParcel::AbstractSecureDataParcel(qint8 type) :
    typeId(type)
{

}

qint8 AbstractSecureDataParcel::getType() const
{
    return typeId;
}

WP::err AbstractSecureDataParcel::writeConfidentData(QDataStream &stream)
{
    stream << (qint8)typeId;
    return WP::kOk;
}

WP::err AbstractSecureDataParcel::readConfidentData(QBuffer &mainData)
{
    QDataStream stream(&mainData);
    qint8 type;
    stream >> type;
    if (type != typeId)
        return WP::kBadValue;
    return WP::kOk;
}
