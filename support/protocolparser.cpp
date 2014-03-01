#include "protocolparser.h"


OutStanza::OutStanza(const QString &name) :
    name(name),
    parent(NULL)
{
}

const QString &OutStanza::getName() const
{
    return name;
}

const QXmlStreamAttributes &OutStanza::getAttributes() const
{
    return attributes;
}

const QString &OutStanza::getText() const
{
    return text;
}

OutStanza *OutStanza::getParent() const
{
    return parent;
}

void OutStanza::setText(const QString &text)
{
    this->text = text;
}

void OutStanza::addAttribute(const QString &namespaceUri, const QString &name, const QString &value)
{
    attributes.append(namespaceUri, name, value);
}

void OutStanza::addAttribute(const QString &qualifiedName, const QString &value)
{
    attributes.append(qualifiedName, value);
}

void OutStanza::clearData()
{
    text = "";
    attributes = QXmlStreamAttributes();
}

void OutStanza::setParent(OutStanza *parent)
{
    this->parent = parent;
}


ProtocolOutStream::ProtocolOutStream(QIODevice *device) :
    xmlWriter(device),
    currentStanza(NULL)
{
    xmlWriter.setAutoFormatting(true);
    xmlWriter.setAutoFormattingIndent(4);
    writeStartDocument();
}

ProtocolOutStream::ProtocolOutStream(QByteArray *data) :
    xmlWriter(data),
    currentStanza(NULL)
{
    writeStartDocument();
}

ProtocolOutStream::~ProtocolOutStream()
{
    while (currentStanza != NULL) {
        OutStanza *parent = currentStanza->getParent();
        delete currentStanza;
        currentStanza = parent;
    }
}

void ProtocolOutStream::pushStanza(OutStanza *stanza)
{
    if (currentStanza != NULL)
        xmlWriter.writeEndElement();
    writeStanze(stanza);

    OutStanza *parent = NULL;
    if (currentStanza != NULL) {
        parent = currentStanza->getParent();
        delete currentStanza;
    }
    stanza->setParent(parent);
    currentStanza = stanza;
}

void ProtocolOutStream::pushChildStanza(OutStanza *stanza)
{
    writeStanze(stanza);

    stanza->setParent(currentStanza);
    currentStanza = stanza;
}

void ProtocolOutStream::cdDotDot()
{
    if (currentStanza != NULL) {
        xmlWriter.writeEndElement();
        OutStanza *parent = currentStanza->getParent();
        delete currentStanza;
        currentStanza = parent;
    }
}

void ProtocolOutStream::flush()
{
    while (currentStanza != NULL) {
        xmlWriter.writeEndElement();
        OutStanza *parent = currentStanza->getParent();
        delete currentStanza;
        currentStanza = parent;
    }
    writeEndDocument();
}

void ProtocolOutStream::writeStanze(OutStanza *stanza)
{
    xmlWriter.writeStartElement(stanza->getName());
    const QXmlStreamAttributes& attributes = stanza->getAttributes();
    for (int i = 0; i < attributes.count(); i++)
        xmlWriter.writeAttribute(attributes.at(i));
    if (stanza->getText() != "")
        xmlWriter.writeCharacters(stanza->getText());
}

void ProtocolOutStream::writeStartDocument()
{
    xmlWriter.writeStartDocument();
}

void ProtocolOutStream::writeEndDocument()
{
    xmlWriter.writeEndDocument();
}


ProtocolInStream::ProtocolInStream(QIODevice *device) :
    xmlReader(device),
    root(NULL)
{
    currentHandlerTree = &root;
    rootHandler = new InStanzaHandler("root", true);
    root.handlers.append(rootHandler);
}

ProtocolInStream::ProtocolInStream(const QByteArray &data) :
    xmlReader(data),
    root(NULL),
    currentHandler(NULL)
{
    currentHandlerTree = &root;
    rootHandler = new InStanzaHandler("root", true);
    root.handlers.append(rootHandler);
}

ProtocolInStream::~ProtocolInStream()
{
}

void ProtocolInStream::parse()
{
    while (!xmlReader.atEnd()) {
        switch (xmlReader.readNext()) {
        case QXmlStreamReader::EndElement: {
            foreach (InStanzaHandler *handler, currentHandlerTree->handlers) {
                if (handler->hasBeenHandled())
                    handler->finished();
            }

            handler_tree *parent = currentHandlerTree->parent;
            delete currentHandlerTree;
            currentHandlerTree = parent;
            break;
        }

        case QXmlStreamReader::StartElement: {
            currentHandler = NULL;
            QString name = xmlReader.name().toString();

            handler_tree *handlerTree = new handler_tree(currentHandlerTree);
            foreach (InStanzaHandler *handler, currentHandlerTree->handlers) {
                foreach (InStanzaHandler *child, handler->getChilds())
                    handlerTree->handlers.append(child);
            }
            currentHandlerTree = handlerTree;

            foreach (InStanzaHandler *handler, currentHandlerTree->handlers) {
               if (handler->stanzaName() == name) {
                    bool handled = handler->handleStanza(xmlReader.attributes());
                    handler->setHandled(handled);
                    if (handled)
                        currentHandler = handler;
                }
            }
            break;
        }

        case QXmlStreamReader::Characters: {
            if (currentHandler == NULL)
                break;
            bool handled = currentHandler->handleText(xmlReader.text());
            currentHandler->setHandled(handled);
            break;
        }

        default:
            break;
        }
    }
}

void ProtocolInStream::addHandler(InStanzaHandler *handler)
{
    rootHandler->addChildHandler(handler);
}


InStanzaHandler::InStanzaHandler(const QString &stanza, bool optional) :
    name(stanza),
    isOptionalStanza(optional),
    stanzaHasBeenHandled(false),
    parent(NULL)
{
}

InStanzaHandler::~InStanzaHandler()
{
    for (int i = 0; i < childHandlers.count(); i++)
        delete childHandlers.at(i);
    childHandlers.clear();
}

QString InStanzaHandler::stanzaName() const
{
    return name;
}

bool InStanzaHandler::isOptional() const
{
    return isOptionalStanza;
}

bool InStanzaHandler::hasBeenHandled() const
{
    if (!stanzaHasBeenHandled)
        return false;

    foreach (InStanzaHandler *child,childHandlers) {
        if (!child->isOptional() && !child->hasBeenHandled())
            return false;
    }

    return true;
}

void InStanzaHandler::setHandled(bool handled)
{
    this->stanzaHasBeenHandled = handled;
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
    childHandlers.append(handler);
    handler->setParent(this);
}

InStanzaHandler *InStanzaHandler::getParent() const
{
    return parent;
}

const QList<InStanzaHandler *> &InStanzaHandler::getChilds() const
{
    return childHandlers;
}

void InStanzaHandler::setParent(InStanzaHandler *parent)
{
    this->parent = parent;
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
    type(type)
{
    addAttribute("type", toString(type));
}


IqType IqOutStanza::getType()
{
    return type;
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
    type(type)
{
}

IqType IqInStanzaHandler::getType()
{
    return type;
}

bool IqInStanzaHandler::handleStanza(const QXmlStreamAttributes &attributes)
{
    if (!attributes.hasAttribute("type"))
        return false;
    IqType stanzaType = fromString(attributes.value("type").toString());
    if (stanzaType != type)
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
