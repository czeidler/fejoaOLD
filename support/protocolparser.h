#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QMap>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


class InStanzaHandler {
public:
    InStanzaHandler(const QString &stanza, bool optional = false);
    virtual ~InStanzaHandler();

    QString stanzaName() const;
    bool isOptional() const;
    bool hasBeenHandled() const;
    void setHandled(bool handled);

    virtual bool handleStanza(const QXmlStreamAttributes &attributes);
    virtual bool handleText(const QStringRef &text);
    virtual void finished();

    void addChildHandler(InStanzaHandler *handler);

    InStanzaHandler *getParent() const;
    const QList<InStanzaHandler *> &getChilds() const;

private:
    void setParent(InStanzaHandler *parent);

    QString name;
    bool isOptionalStanza;
    bool stanzaHasBeenHandled;

    InStanzaHandler *parent;
    QList<InStanzaHandler*> childHandlers;
};


class ProtocolInStream {
public:
    ProtocolInStream(QIODevice *device);
    ProtocolInStream(const QByteArray &data);
    ~ProtocolInStream();

    void parse();

    void addHandler(InStanzaHandler *handler);

private:
    struct handler_tree {
        handler_tree(handler_tree *parent);
        ~handler_tree();

        handler_tree *parent;
        QList<InStanzaHandler*> handlers;
    };

    QXmlStreamReader xmlReader;
    handler_tree root;
    InStanzaHandler *rootHandler;
    InStanzaHandler *currentHandler;
    handler_tree *currentHandlerTree;
};

class OutStanza {
public:
    OutStanza(const QString &getName);

    const QString &getName() const;
    const QXmlStreamAttributes &getAttributes() const;
    const QString &getText() const;
    OutStanza *getParent() const;

    void setText(const QString &getText);
    void addAttribute(const QString &namespaceUri, const QString &getName, const QString &value);
    void addAttribute(const QString &qualifiedName, const QString &value);

    /*! Clear all data except the stanza name. This can be used to free memory of large text in
     *  nested stanza trees. */
    void clearData();

    void setParent(OutStanza *getParent);
private:
    QString name;
    QXmlStreamAttributes attributes;
    QString text;
    OutStanza *parent;
};

class ProtocolOutStream {
public:
    ProtocolOutStream(QIODevice* device);
    ProtocolOutStream(QByteArray* data);
    ~ProtocolOutStream();

    void pushStanza(OutStanza *stanza);
    void pushChildStanza(OutStanza *stanza);
    void cdDotDot();
    void flush();

private:
    void writeStanze(OutStanza *stanza);
    void writeStartDocument();
    void writeEndDocument();

    OutStanza *currentStanza;
    QXmlStreamWriter xmlWriter;
};


// special stanzas

enum IqType {
    kGet,
    kSet,
    kResult,
    kError,
    kBadType
};


class IqOutStanza : public OutStanza {
public:
    IqOutStanza(IqType getType);

    IqType getType();

private:
    static QString toString(IqType type);

    IqType type;
};

class IqInStanzaHandler : public InStanzaHandler {
public:
    IqInStanzaHandler(IqType getType);

    IqType getType();
    virtual bool handleStanza(const QXmlStreamAttributes &attributes);

private:
    static IqType fromString(const QString &string);

    IqType type;
};

#endif // PROTOCOLPARSER_H
