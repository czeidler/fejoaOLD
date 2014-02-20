#include "messagereceiver.h"

#include <stdio.h>

#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


class XMLHandler {
public:
    XMLHandler(const QByteArray& input, MessageReceiver* receiver)
        :
        fXML(input),
        fMessageReceiver(receiver)
    {
    }

    void handle() {
        while (!fXML.atEnd()) {
            switch (fXML.readNext()) {
            case QXmlStreamReader::EndElement:
                break;
            case QXmlStreamReader::StartElement:
                if (fXML.name().compare("iq", Qt::CaseInsensitive) == 0) {
                    QXmlStreamAttributes attributes = fXML.attributes();
                    if (attributes.hasAttribute("type")) {
                        if (attributes.value("type").compare("result", Qt::CaseInsensitive) == 0)
                            handleIqResult();
                    }

                }
                break;
            default:
                break;
            }
        }

    }

    void handleIqResult() {
        while (!fXML.atEnd()) {
            switch (fXML.readNext()) {
            case QXmlStreamReader::EndElement:
                return;
                break;
            case QXmlStreamReader::StartElement:
                if (fXML.name().compare("query", Qt::CaseInsensitive) == 0) {
                    handleIqResultQuery();
                }
                break;
            default:
                break;
            }
        }

    }

    void handleIqResultQuery() {
        QByteArray packedData;
        QString branch;
        QString first;
        QString last;
        while (!fXML.atEnd()) {
            switch (fXML.readNext()) {
            case QXmlStreamReader::EndElement:
                return;
                break;
            case QXmlStreamReader::StartElement:
                if (fXML.name().compare("pack", Qt::CaseInsensitive) == 0) {
                    QXmlStreamAttributes attributes = fXML.attributes();
                    if (attributes.hasAttribute("branch"))
                        branch = attributes.value("branch").toString();
                    if (attributes.hasAttribute("first"))
                        first = attributes.value("first").toString();
                    if (attributes.hasAttribute("last"))
                        last = attributes.value("last").toString();
                    //TODO handle different package formats...
                }
                break;
            case QXmlStreamReader::Characters:
                packedData = fXML.text().toLocal8Bit();
                fMessageReceiver->commitPackReceived(packedData, branch, first, last);
                break;
            default:
                break;
            }
        }

    }


private:
        QXmlStreamReader       fXML;
        MessageReceiver*       fMessageReceiver;
};

MessageReceiver::MessageReceiver(GitInterface* gitInterface, QObject *parent) :
    QObject(parent),
    fGitInterface(gitInterface)
{
}

void MessageReceiver::messageDataReceived(const QByteArray& data)
{
    XMLHandler xmlHandler(data, this);
    xmlHandler.handle();
}

QString MessageReceiver::getMessagesRequest()
{
    QString first = fGitInterface->getTip();
    return _CreateXMLMessageRequest("master", first, "");
}

void MessageReceiver::commitPackReceived(const QByteArray& data, const QString& branch,
                                         const QString &/*first*/, const QString &last)
{
    QByteArray text = QByteArray::fromBase64(data);

    int objectStart = 0;
    while (objectStart < text.length()) {
        QString hash;
        QString size;
        int objectEnd = objectStart;
        objectEnd = _ReadTill(text, hash, objectEnd, ' ');
        printf("hash %s\n", hash.toStdString().c_str());
        objectEnd = _ReadTill(text, size, objectEnd, '\0');
        int blobStart = objectEnd;
        printf("size %i\n", size.toInt());
        objectEnd += size.toInt();

        const char* dataPointer = text.data() + blobStart;
        fGitInterface->writeFile(hash, dataPointer, objectEnd - blobStart);

        objectStart = objectEnd;
    }

    // update tip
    fGitInterface->updateTip(last);
}


int MessageReceiver::_ReadTill(QByteArray& in, QString &out, int start, char stopChar)
{
    int pos = start;
    while (pos < in.length() && in.at(pos) != stopChar) {
        out.append(in.at(pos));
        pos++;
    }
    pos++;
    return pos;
}

QString MessageReceiver::_CreateXMLMessageRequest(const QString& branch, const QString &fromCommit, const QString &toCommit)
{
    QString output;
    QXmlStreamWriter xmlWriter(&output);
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("iq");
    xmlWriter.writeAttribute("type", "get");
    xmlWriter.writeStartElement("query");
    xmlWriter.writeAttribute("xmlns", "git:transfer");
    xmlWriter.writeStartElement("commits");
    xmlWriter.writeAttribute("branch", branch);
    xmlWriter.writeAttribute("first", fromCommit);
    xmlWriter.writeAttribute("last", toCommit);
    xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();
    printf("xml: %s\n", output.toStdString().c_str());
    return output;
}

