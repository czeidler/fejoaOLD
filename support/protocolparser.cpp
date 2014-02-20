#include "protocolparser.h"


OutStanza::OutStanza(const QString &name) :
    fName(name),
    fParent(NULL)
{
}

const QString &OutStanza::name() const
{
    return fName;
}

const QXmlStreamAttributes &OutStanza::attributes() const
{
    return fAttributes;
}

const QString &OutStanza::text() const
{
    return fText;
}

OutStanza *OutStanza::parent() const
{
    return fParent;
}

void OutStanza::setText(const QString &text)
{
    fText = text;
}

void OutStanza::addAttribute(const QString &namespaceUri, const QString &name, const QString &value)
{
    fAttributes.append(namespaceUri, name, value);
}

void OutStanza::addAttribute(const QString &qualifiedName, const QString &value)
{
    fAttributes.append(qualifiedName, value);
}

void OutStanza::clearData()
{
    fText = "";
    fAttributes = QXmlStreamAttributes();
}

void OutStanza::setParent(OutStanza *parent)
{
    fParent = parent;
}


ProtocolOutStream::ProtocolOutStream(QIODevice *device) :
    fXMLWriter(device),
    fCurrentStanza(NULL)
{
    fXMLWriter.setAutoFormatting(true);
    fXMLWriter.setAutoFormattingIndent(4);
    writeStartDocument();
}

ProtocolOutStream::ProtocolOutStream(QByteArray *data) :
    fXMLWriter(data),
    fCurrentStanza(NULL)
{
    writeStartDocument();
}

ProtocolOutStream::~ProtocolOutStream()
{
    while (fCurrentStanza != NULL) {
        OutStanza *parent = fCurrentStanza->parent();
        delete fCurrentStanza;
        fCurrentStanza = parent;
    }
}

void ProtocolOutStream::pushStanza(OutStanza *stanza)
{
    if (fCurrentStanza != NULL)
        fXMLWriter.writeEndElement();
    writeStanze(stanza);

    OutStanza *parent = NULL;
    if (fCurrentStanza != NULL) {
        parent = fCurrentStanza->parent();
        delete fCurrentStanza;
    }
    stanza->setParent(parent);
    fCurrentStanza = stanza;
}

void ProtocolOutStream::pushChildStanza(OutStanza *stanza)
{
    writeStanze(stanza);

    stanza->setParent(fCurrentStanza);
    fCurrentStanza = stanza;
}

void ProtocolOutStream::cdDotDot()
{
    if (fCurrentStanza != NULL) {
        fXMLWriter.writeEndElement();
        OutStanza *parent = fCurrentStanza->parent();
        delete fCurrentStanza;
        fCurrentStanza = parent;
    }
}

void ProtocolOutStream::flush()
{
    while (fCurrentStanza != NULL) {
        fXMLWriter.writeEndElement();
        OutStanza *parent = fCurrentStanza->parent();
        delete fCurrentStanza;
        fCurrentStanza = parent;
    }
    writeEndDocument();
}

void ProtocolOutStream::writeStanze(OutStanza *stanza)
{
    fXMLWriter.writeStartElement(stanza->name());
    const QXmlStreamAttributes& attributes = stanza->attributes();
    for (int i = 0; i < attributes.count(); i++)
        fXMLWriter.writeAttribute(attributes.at(i));
    if (stanza->text() != "")
        fXMLWriter.writeCharacters(stanza->text());
}

void ProtocolOutStream::writeStartDocument()
{
    fXMLWriter.writeStartDocument();
}

void ProtocolOutStream::writeEndDocument()
{
    fXMLWriter.writeEndDocument();
}


ProtocolInStream::ProtocolInStream(QIODevice *device) :
    fXMLReader(device),
    fRoot(NULL)
{
    fCurrentHandlerTree = &fRoot;
    fRootHandler = new InStanzaHandler("root", true);
    fRoot.handlers.append(fRootHandler);
}

ProtocolInStream::ProtocolInStream(const QByteArray &data) :
    fXMLReader(data),
    fRoot(NULL),
    fCurrentHandler(NULL)
{
    fCurrentHandlerTree = &fRoot;
    fRootHandler = new InStanzaHandler("root", true);
    fRoot.handlers.append(fRootHandler);
}

ProtocolInStream::~ProtocolInStream()
{
}

void ProtocolInStream::parse()
{
    while (!fXMLReader.atEnd()) {
        switch (fXMLReader.readNext()) {
        case QXmlStreamReader::EndElement: {
            foreach (InStanzaHandler *handler, fCurrentHandlerTree->handlers) {
                if (handler->hasBeenHandled())
                    handler->finished();
            }

            handler_tree *parent = fCurrentHandlerTree->parent;
            delete fCurrentHandlerTree;
            fCurrentHandlerTree = parent;
            break;
        }

        case QXmlStreamReader::StartElement: {
            fCurrentHandler = NULL;
            QString name = fXMLReader.name().toString();

            handler_tree *handlerTree = new handler_tree(fCurrentHandlerTree);
            foreach (InStanzaHandler *handler, fCurrentHandlerTree->handlers) {
                foreach (InStanzaHandler *child, handler->childs())
                    handlerTree->handlers.append(child);
            }
            fCurrentHandlerTree = handlerTree;

            foreach (InStanzaHandler *handler, fCurrentHandlerTree->handlers) {
               if (handler->stanzaName() == name) {
                    bool handled = handler->handleStanza(fXMLReader.attributes());
                    handler->setHandled(handled);
                    if (handled)
                        fCurrentHandler = handler;
                }
            }
            break;
        }

        case QXmlStreamReader::Characters: {
            if (fCurrentHandler == NULL)
                break;
            bool handled = fCurrentHandler->handleText(fXMLReader.text());
            fCurrentHandler->setHandled(handled);
            break;
        }

        default:
            break;
        }
    }
}

void ProtocolInStream::addHandler(InStanzaHandler *handler)
{
    fRootHandler->addChildHandler(handler);
}


InStanzaHandler::InStanzaHandler(const QString &stanza, bool optional) :
    fName(stanza),
    fIsOptional(optional),
    fHasBeenHandled(false),
    fParent(NULL)
{
}

InStanzaHandler::~InStanzaHandler()
{
    for (int i = 0; i < fChildHandlers.count(); i++)
        delete fChildHandlers.at(i);
    fChildHandlers.clear();
}

QString InStanzaHandler::stanzaName() const
{
    return fName;
}

bool InStanzaHandler::isOptional() const
{
    return fIsOptional;
}

bool InStanzaHandler::hasBeenHandled() const
{
    if (!fHasBeenHandled)
        return false;

    foreach (InStanzaHandler *child,fChildHandlers) {
        if (!child->isOptional() && !child->hasBeenHandled())
            return false;
    }

    return true;
}

void InStanzaHandler::setHandled(bool handled)
{
    fHasBeenHandled = handled;
}

bool InStanzaHandler::handleStanza(const QXmlStreamAttributes &/*attributes*/)
{
    return false;
}

bool InStanzaHandler::handleText(const QStringRef &/*text*/)
{
    return false;
}

void InStanzaHandler::finished()
{
}

void InStanzaHandler::addChildHandler(InStanzaHandler *handler)
{
    fChildHandlers.append(handler);
    handler->setParent(this);
}

InStanzaHandler *InStanzaHandler::parent() const
{
    return fParent;
}

const QList<InStanzaHandler *> &InStanzaHandler::childs() const
{
    return fChildHandlers;
}

void InStanzaHandler::setParent(InStanzaHandler *parent)
{
    fParent = parent;
}

ProtocolInStream::handler_tree::handler_tree(ProtocolInStream::handler_tree *p) :
    parent(p)
{
}

ProtocolInStream::handler_tree::~handler_tree()
{
}


IqOutStanza::IqOutStanza(IqType type) :
    OutStanza("iq"),
    fType(type)
{
    addAttribute("type", toString(fType));
}


IqType IqOutStanza::type()
{
    return fType;
}

QString IqOutStanza::toString(IqType type)
{
    switch (type) {
    case kGet:
        return "get";
    case kSet:
        return "set";
    case kResult:
        return "result";
    case kError:
        return "error";
    default:
        return "bad_type";
    }
    return "";
}


IqInStanzaHandler::IqInStanzaHandler(IqType type) :
    InStanzaHandler("iq"),
    fType(type)
{
}

IqType IqInStanzaHandler::type()
{
    return fType;
}

bool IqInStanzaHandler::handleStanza(const QXmlStreamAttributes &attributes)
{
    if (!attributes.hasAttribute("type"))
        return false;
    IqType type = fromString(attributes.value("type").toString());
    if (type != fType)
        return false;
    return true;
}

IqType IqInStanzaHandler::fromString(const QString &string)
{
    if (string.compare("get", Qt::CaseInsensitive) == 0)
        return kGet;
    if (string.compare("set", Qt::CaseInsensitive) == 0)
        return kSet;
    if (string.compare("result", Qt::CaseInsensitive) == 0)
        return kResult;
    if (string.compare("error", Qt::CaseInsensitive) == 0)
        return kError;
    return kBadType;
}
