#ifndef MAIL_H
#define MAIL_H

#include <QBuffer>

#include "contact.h"
#include "protocolparser.h"



class ParcelCrypto {
public:
    void initNew();
    WP::err initFromPublic(Contact *receiver, const QString &keyId, const QByteArray &iv, const QByteArray &encryptedSymmetricKey);
    void initFromPrivate(const QByteArray &iv, const QByteArray &symmetricKey);

    WP::err cloakData(const QByteArray &data, QByteArray &cloakedData);
    WP::err uncloakData(const QByteArray &cloakedData, QByteArray &data);

    const QByteArray &getIV() const;
    const QByteArray &getSymmetricKey() const;
    WP::err getEncryptedSymmetricKey(Contact *receiver, const QString &keyId, QByteArray &encryptedSymmetricKey);

protected:
    const static int kSymmetricKeySize = 256;

    QByteArray iv;
    QByteArray symmetricKey;
};


class DataParcel {
public:
    virtual ~DataParcel() {}

    const QByteArray getSignature() const;
    const QString getSignatureKey() const;
    const Contact *getSender() const;

    virtual QString getUid() const;

    virtual WP::err toRawData(Contact *sender, const QString &signatureKey, QIODevice &rawData);
    virtual WP::err fromRawData(ContactFinder *contactFinder, QByteArray &rawData);

protected:
    virtual WP::err writeMainData(QDataStream &stream) = 0;
    virtual WP::err readMainData(QBuffer &mainData) = 0;

    QString readString(QIODevice &data) const;

protected:
    QByteArray signature;
    QString signatureKey;
    Contact *sender;

    QString uid;
};


class SecureChannel;


class AbstractSecureDataParcel : public DataParcel {
public:
    AbstractSecureDataParcel(qint8 type);

    qint8 getType() const;

protected:
    virtual WP::err writeConfidentData(QDataStream &stream);
    virtual WP::err readConfidentData(QBuffer &mainData);

private:
    qint8 typeId;
};

class SecureChannelParcel : public AbstractSecureDataParcel {
public:
    // outgoing
    SecureChannelParcel(qint8 type, SecureChannel *channel = NULL);

    void setChannel(SecureChannel *channel);
    SecureChannel *getChannel() const;

    time_t getTimestamp() const;

protected:
    virtual WP::err writeMainData(QDataStream &stream);
    virtual WP::err readMainData(QBuffer &mainData);

    virtual WP::err writeConfidentData(QDataStream &stream);
    virtual WP::err readConfidentData(QBuffer &mainData);

    virtual SecureChannel *findChannel(const QString &channelUid) = 0;

protected:
    SecureChannel *channel;

private:
    time_t timestamp;
};


class SecureChannel : public AbstractSecureDataParcel {
public:
    /*! Use existing channel but use a a different receiver.
     */
    SecureChannel(SecureChannel *channel, Contact *receiver, const QString &asymKeyId);
    // incoming
    SecureChannel(qint8 type, Contact *receiver);
    // outgoing
    SecureChannel(qint8 type, Contact *receiver, const QString &asymKeyId);
    virtual ~SecureChannel();

    /*! The id should be the same for all users. Thus the asym part can't be part of the hash.
    At the moment the channel id is just the sha1 hash of the iv.
    */
    virtual QString getUid() const;

    virtual WP::err writeDataSecure(QDataStream &stream, const QByteArray &data);
    virtual WP::err readDataSecure(QDataStream &stream, QByteArray &data);

protected:
    virtual WP::err writeMainData(QDataStream &stream);
    virtual WP::err readMainData(QBuffer &mainData);

private:
    Contact *receiver;

    QString asymmetricKeyId;
    ParcelCrypto parcelCrypto;
};


enum parcel_identifiers {
    kMessageChannelId = 1,
    kMessageId,
    kMessageChannelInfoId
};


class MessageChannel;
class MessageChannelInfo;
class MessageParcel;

class MessageChannelFinder {
public:
    virtual ~MessageChannelFinder() {}
    virtual MessageChannel *findChannel(const QString &channelUid) = 0;
    virtual MessageChannelInfo *findChannelInfo(const QString &channelUid,
                                            const QString &channelInfoUid) = 0;
};


class MessageChannelInfo : public SecureChannelParcel {
public:
    MessageChannelInfo(MessageChannelFinder *channelFinder);
    MessageChannelInfo(MessageChannel *channel);
    ~MessageChannelInfo();

    void setSubject(const QString &subject);
    const QString &getSubject() const;

    void addParticipant(const QString &address, const QString &uid);
    bool setParticipantUid(const QString &address, const QString &uid);

    bool isNewLocale() const;

    class Participant {
    public:
        QString address;
        QString uid;
    };

    QVector<Participant> &getParticipants();

protected:
    virtual WP::err writeConfidentData(QDataStream &stream);
    virtual WP::err readConfidentData(QBuffer &mainData);

    virtual SecureChannel *findChannel(const QString &channelUid);

private:
    WP::err readParticipants(QDataStream &stream);

private:
    enum {
        kSubject = 1,
        kParticipants
    };

    MessageChannelFinder *channelFinder;
    QString subject;
    QVector<Participant> participants;
    bool newLocaleInfo;
};


class MessageChannel : public SecureChannel {
public:
    //! adapt channel for different receiver:
    MessageChannel(MessageChannel *channel, Contact *receiver, const QString &asymKeyId);
    // incoming:
    MessageChannel(Contact *receiver);
    // outgoing:
    MessageChannel(Contact *receiver, const QString &asymKeyId, MessageChannel *parentChannel = NULL);

    QString getParentChannelUid() const;

    bool isNewLocale() const;

protected:
    virtual WP::err writeMainData(QDataStream &stream);
    virtual WP::err readMainData(QBuffer &mainData);

    virtual WP::err writeConfidentData(QDataStream &stream);
    virtual WP::err readConfidentData(QBuffer &mainData);

private:
    QString parentChannelUid;

    bool newLocaleInfo;
};


class Message : public SecureChannelParcel {
public:
    Message(MessageChannelFinder *channelFinder);
    Message(MessageChannelInfo *info);
    ~Message();

    MessageChannelInfo *getChannelInfo() const;

    const QByteArray& getBody() const;
    void setBody(const QByteArray &body);

protected:
    virtual WP::err writeConfidentData(QDataStream &stream);
    virtual WP::err readConfidentData(QBuffer &mainData);

    virtual SecureChannel *findChannel(const QString &channelUid);

private:
    QByteArray body;
    MessageChannelInfo *channelInfo;
    MessageChannelFinder *channelFinder;
};


class XMLSecureParcel {
public:
    static WP::err write(ProtocolOutStream *outStream, Contact *sender,
                         const QString &signatureKeyId, DataParcel *parcel,
                         const QString &stanzaName);
};


#endif // MAIL_H
