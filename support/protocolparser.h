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

    InStanzaHandler *parent() const;
    const QList<InStanzaHandler *> &childs() const;

private:
    void setParent(InStanzaHandler *parent);

    QString fName;
    bool fIsOptional;
    bool fHasBeenHandled;

    InStanzaHandler *fParent;
    QList<InStanzaHandler*> fChildHandlers;
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

    QXmlStreamReader fXMLReader;
    handler_tree fRoot;
    InStanzaHandler *fRootHandler;
    InStanzaHandler *fCurrentHandler;
    handler_tree *fCurrentHandlerTree;
};

class OutStanza {
public:
    OutStanza(const QString &name);

    const QString &name() const;
    const QXmlStreamAttributes &attributes() const;
    const QString &text() const;
    OutStanza *parent() const;

    void setText(const QString &text);
    void addAttribute(const QString &namespaceUri, const QString &name, const QString &value);
    void addAttribute(const QString &qualifiedName, const QString &value);

    /*! Clear all data except the stanza name. This can be used to free memory of large text in
     *  nested stanza trees. */
    void clearData();

    void setParent(OutStanza *parent);
private:
    QString fName;
    QXmlStreamAttributes fAttributes;
    QString fText;
    OutStanza *fParent;
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

    OutStanza *fCurrentStanza;
    QXmlStreamWriter fXMLWriter;
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
    IqOutStanza(IqType type);

    IqType type();

private:
    static QString toString(IqType type);

    IqType fType;
};

class IqInStanzaHandler : public InStanzaHandler {
public:
    IqInStanzaHandler(IqType type);

    IqType type();
    virtual bool handleStanza(const QXmlStreamAttributes &attributes);

private:
    static IqType fromString(const QString &string);

    IqType fType;
};

#endif // PROTOCOLPARSER_H
